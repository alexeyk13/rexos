/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#include "nrf_rf.h"
#include "nrf_power.h"
#include "../../userspace/sys.h"
#include "../../userspace/nrf/nrf_radio.h"
#include "../../userspace/nrf/nrf_ble.h"
#include "../../userspace/nrf/nrf_driver.h"
#include "../kerror.h"
#include "../kstdlib.h"
#include "../ksystime.h"
#include "../kstream.h"
#include "../kirq.h"
#include "../kexo.h"
#include <string.h>
#include "nrf_exo_private.h"
#include "sys_config.h"

#define NRF_MAX_PACKET_LENGTH                   254

static const uint8_t template_adv_packet[] = {
        0x42, // ADV_NONCONN_IND
        0x14, // LENGTH
//        0x00,
        0xF0, // MAC
        0x4C,
        0x16,
        0x48,
        0xF0,
        0xEA,
        0x02, // COMMON DATA
        0x01,
        0x06,
        0x07, // NAME LENGTH
        0x09, // type name
        'R',
        'J',
        'T',
        'E',
        'S',
        'T',
        0x55, // CRC
        0x55,
        0x55
};

static const uint8_t rsp[] = {
        0x44,
        0x06,
//        0x00,
        //------------------------------
        0xF0, // MAC
        0x4C,
        0x16,
        0x48,
        0xF0,
        0xEA,
        //------------------------------
        0x55, // CRC
        0x55,
        0x55
};

/** Return 2^n, used for setting nth bit as 1*/
#define SET_BIT(n)      (1UL << n)

static inline void collect_packet(EXO* exo)
{
    uint8_t pkt_size = exo->rf.pdu[1] + 2;
    /* Collect a max of LOGGER_NUMBER packets, otherwise reduce observer window */
    if(exo->rf.io->data_size + pkt_size < exo->rf.max_size)
    {
        io_data_append(exo->rf.io, exo->rf.pdu, pkt_size);
        io_data_append(exo->rf.io, &(exo->rf.rssi), sizeof(uint32_t));

//        exo->rf.packets[exo->rf.packet_count][MAX_PDU_SIZE] = NRF_RADIO->DATAWHITEIV - 64;  //6th bit is always 1
//
//        if(NRF_RADIO->CRCSTATUS == 1)
//        {
//            exo->rf.packets[exo->rf.packet_count][MAX_PDU_SIZE] |= 0x80;
//        }
    }
}

static inline void nrf_rf_irq(int vector, void* param)
{
    EXO* exo = (EXO*)param;

//    printk("--> i\n");

    /* disable timer */
    ksystime_soft_timer_stop(exo->rf.timer);

    /* parse irq */
    switch(exo->rf.state)
    {
        case RADIO_STATE_RX:
        {
            if((NRF_RADIO->EVENTS_END == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk))
                NRF_RADIO->EVENTS_END = 0;
            /* save data size to io */
            /* pkt_size is second byte plus 2 preamble */
            exo->rf.io->data_size = ((uint8_t*)io_data(exo->rf.io))[1] + 2;

            if((NRF_RADIO->EVENTS_RSSIEND == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_RSSIEND_Msk))
            {
                NRF_RADIO->EVENTS_RSSIEND = 0;
                NRF_RADIO->TASKS_RSSISTOP = 1;
                exo->rf.rssi = (NRF_RADIO->RSSISAMPLE);
                io_data_append(exo->rf.io, &exo->rf.rssi, sizeof(uint32_t));
            }
        }
        // follow down
        case RADIO_STATE_TX:
            /* disable task */
            NRF_RADIO->TASKS_DISABLE = 1;
            /* send reply back */
            iio_complete(exo->rf.process,
                         HAL_IO_CMD(HAL_RF, (exo->rf.state == RADIO_STATE_RX) ? IPC_READ : IPC_WRITE),
                         0,
                         exo->rf.io);

            exo->rf.io = NULL;
            exo->rf.process = INVALID_HANDLE;
            exo->rf.state = RADIO_STATE_IDLE;
            break;
        default:
            break;
    }

//    printk("<-- i\n");
}

void nrf_rf_init(EXO* exo)
{
    exo->rf.active = false;
    exo->rf.io = NULL;
    exo->rf.process = INVALID_HANDLE;
    exo->rf.timer = INVALID_HANDLE;
    exo->rf.max_size = 0;
    exo->rf.state = RADIO_STATE_IDLE;
}

static void nrf_rf_start(EXO* exo, HANDLE process, IO* io)
{
//    printk("rf start\n");
    ksystime_soft_timer_start_ms(exo->rf.timer, 3000);
    exo->rf.process = process;
    exo->rf.io = io;
    kerror(ERROR_SYNC);
}

static void nrf_rf_stop(EXO* exo)
{

}

static inline void nrf_rf_setup_mode(EXO* exo, RADIO_MODE mode)
{
//    printk("rf mode: %d\n", mode);

    /* set same mode */
    if(exo->rf.mode == mode)
        return;

    /* set mode */
    exo->rf.mode = mode;

    switch(mode)
    {
        case RADIO_MODE_RF_250Kbit:
            /* set RADIO mode to RF 250 kBit */
            NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_250Kbit << RADIO_MODE_MODE_Pos;

            /* Configure Access Address according to the BLE standard */
            NRF_RADIO->PREFIX0 = 0x00;
            NRF_RADIO->BASE0 = 0x00;

            /* Data whitening */
            NRF_RADIO->DATAWHITEIV = 0x00;

            /* PCNF-> Packet Configuration.
             * We now need to configure the sizes S0, S1 and length field to match the
             * datapacket format of the advertisement packets.
             */
            NRF_RADIO->PCNF0 = (
              (((1UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) |  /* Length of S0 field in bytes 0-1.    */
              (((2UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) |  /* Length of S1 field in bits 0-8.     */
              (((6UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk)    /* Length of length field in bits 0-8. */
              );

            /* Packet configuration */
            NRF_RADIO->PCNF1 = (
              (((37UL) << RADIO_PCNF1_MAXLEN_Pos) & RADIO_PCNF1_MAXLEN_Msk)   |                      /* Maximum length of payload in bytes [0-255] */
              (((0UL) << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk)   |                      /* Expand the payload with N bytes in addition to LENGTH [0-255] */
              (((3UL) << RADIO_PCNF1_BALEN_Pos) & RADIO_PCNF1_BALEN_Msk)       |                      /* Base address length in number of bytes. */
              (((RADIO_PCNF1_ENDIAN_Little) << RADIO_PCNF1_ENDIAN_Pos) & RADIO_PCNF1_ENDIAN_Msk) |  /* Endianess of the S0, LENGTH, S1 and PAYLOAD fields. */
              (((1UL) << RADIO_PCNF1_WHITEEN_Pos) & RADIO_PCNF1_WHITEEN_Msk)                         /* Enable packet whitening */
            );

            break;
        case RADIO_MODE_RF_1Mbit:
            // TODO:
            printk("not implemented\n");
            kerror(ERROR_NOT_SUPPORTED);
            break;
        case RADIO_MODE_RF_2Mbit:
            // TODO:
            printk("not implemented\n");
            kerror(ERROR_NOT_SUPPORTED);
            break;
        case RADIO_MODE_BLE_1Mbit:
            /* set RADIO mode to Bluetooth Low Energy. */
            NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

            /* copy the BLE override registers from FICR */
            NRF_RADIO->OVERRIDE0 =  NRF_FICR->BLE_1MBIT[0];
            NRF_RADIO->OVERRIDE1 =  NRF_FICR->BLE_1MBIT[1];
            NRF_RADIO->OVERRIDE2 =  NRF_FICR->BLE_1MBIT[2];
            NRF_RADIO->OVERRIDE3 =  NRF_FICR->BLE_1MBIT[3];
            NRF_RADIO->OVERRIDE4 =  NRF_FICR->BLE_1MBIT[4];

            /* Set access address to 0x8E89BED6. This is the access address to be used
            * when send packets in obsvrertise channels.
            *
            * Since the access address is 4 bytes long and the prefix is 1 byte long,
            * we first set the base address length to be 3 bytes long.
            *
            * Then we split the full access address in:
            * 1. Prefix0:  0x0000008E (LSB -> Logic address 0)
            * 2. Base0:    0x89BED600 (3 MSB)
            *
            * At last, we enable reception for this address.
            */
           NRF_RADIO->PCNF1        |= 3UL << RADIO_PCNF1_BALEN_Pos;
           NRF_RADIO->BASE0        = 0x89BED600;
           NRF_RADIO->PREFIX0      = 0x0000008E;
           NRF_RADIO->RXADDRESSES  = 0x00000001;

           /* Configure CRC.
               *
               * First, we set the length of CRC field to 3 bytes long and ignore the
               * access address in the CRC calculation.
               *
               * Then we set CRC initial value to 0x555555.
               *
               * The last step is to set the CRC polynomial to
               * x^24 + x^10 + x^9 + x^6 + x^4 + x^3 + x + 1.
               */
           NRF_RADIO->CRCCNF =     RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos |
                                  RADIO_CRCCNF_SKIP_ADDR_Skip
                                                  << RADIO_CRCCNF_SKIP_ADDR_Pos;
           NRF_RADIO->CRCINIT =    0x555555UL;
           NRF_RADIO->CRCPOLY =    SET_BIT(24) | SET_BIT(10) | SET_BIT(9) |
                                      SET_BIT(6) | SET_BIT(4) | SET_BIT(3) |
                                      SET_BIT(1) | SET_BIT(0);

            /* Configure header size.
               *
               * The Advertise has the following format:
               * RxAdd(1b) | TxAdd(1b) | RFU(2b) | PDU Type(4b) | RFU(2b) | Length(6b)
               *
               * And the nRF51822 RADIO packet has the following format
               * (directly editable fields):
               * S0 (0/1 bytes) | LENGTH ([0, 8] bits) | S1 ([0, 8] bits)
               *
               * We can match those fields with the Link Layer fields:
               * S0 (1 byte)      --> PDU Type(4bits)|RFU(2bits)|TxAdd(1bit)|RxAdd(1bit)
               * LENGTH (6 bits)  --> Length(6bits)
               * S1 (0 bits)      --> S1(0bits)
               */
           NRF_RADIO->PCNF0 |= (1 << RADIO_PCNF0_S0LEN_Pos) |  /* 1 byte */
                                  (8 << RADIO_PCNF0_LFLEN_Pos) |  /* 6 bits */
                                  (0 << RADIO_PCNF0_S1LEN_Pos);   /* 2 bits */

            /* Configure the shorts for observing
             * READY event and START task
             * ADDRESS event and RSSISTART task
             * END event and START task
                 * */

           NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
                                    RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
                                    RADIO_SHORTS_END_START_Msk;
            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
            return;
    }

}

static inline void nrf_rf_open(EXO* exo, RADIO_MODE mode)
{
    // TODO: remove me
//    printk("rf open\n");
    /* already configured */
    if(exo->rf.active)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    /* create timer */
    exo->rf.timer = ksystime_soft_timer_create(KERNEL_HANDLE, 0, HAL_RF);
    if(exo->rf.timer == INVALID_HANDLE)
    {
        kerror(ERROR_OUT_OF_MEMORY);
        return;
    }

    /* RADIO peripheral power control ON */
    NRF_RADIO->POWER = 1;

    /* Set radio transmit power to default 0dBm */
    NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);

    /* setup mode */
    nrf_rf_setup_mode(exo, mode);

    /* setup IRQ */
    kirq_register(KERNEL_HANDLE, RADIO_IRQn, nrf_rf_irq, (void*)exo);
    // Enable Interrupt for RADIO in the core.
    NVIC_SetPriority(RADIO_IRQn, 2);

    exo->rf.active = true;

    // TODO: remove me
//    printk("rf open complete\n");
}

static void nrf_rf_close(EXO* exo)
{
//    printk("rf close\n");
}

static void nrf_rf_io(EXO* exo, HANDLE process, HANDLE user, IO* io, unsigned int size, bool rx)
{
    RADIO_STACK* stack = io_stack(io);
    io_pop(io, sizeof(RADIO_STACK));

    // TODO:
//    printk("nrf rf io\n");
//    printk("    %s, size %d\n", (rx)? "RX" : "TX", size);

    if(!exo->rf.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    exo->rf.io = io;
    exo->rf.process = process;

    /* flush events */
    NRF_RADIO->EVENTS_READY = 0;

    /* prepare pkt data */
    NRF_RADIO->PACKETPTR = (uint32_t)io_data(io);

    if(RADIO_MODE_BLE_1Mbit == exo->rf.mode)
    {
        /* set data whiteng for BLE depending on channel */
        NRF_RADIO->DATAWHITEIV = (NRF_RADIO->FREQUENCY == 2) ? 37 : ((NRF_RADIO->FREQUENCY == 26) ? 38 : 39);
    }

    if(rx)
    {
        exo->rf.state = RADIO_STATE_RX;
        /* Before the RADIO is able to receive a packet, it must first ramp-up in RX mode */
        NRF_RADIO->TASKS_RXEN = 1;
    }
    else
    {
        exo->rf.state = RADIO_STATE_TX;
        /* Before the RADIO is able to transmit a packet, it must first ramp-up in TX mode */
        NRF_RADIO->TASKS_TXEN = 1;
    }

    if(!(RADIO_MODE_BLE_1Mbit == exo->rf.mode))
    {
        /* wait ramp-up sequence is initiated */
        while(NRF_RADIO->EVENTS_READY == 0)
        {
            __NOP();
            __NOP();
        }
    }

    /* enable interrupt */
    NRF_RADIO->INTENSET = RADIO_INTENSET_RSSIEND_Msk |
                          RADIO_INTENSET_END_Msk;

    /* flush events end */
    NRF_RADIO->EVENTS_END  = 0;

    /* enable IRQ */
    NVIC_EnableIRQ(RADIO_IRQn);

    /* setup timeout */
    if(stack->flags & RADIO_FLAG_TIMEOUT)
        ksystime_soft_timer_start_ms(exo->rf.timer, stack->timeout_ms);

    /* Start rx or tx */
    NRF_RADIO->TASKS_START = 1;
    /* wait events in irq */
    kerror(ERROR_SYNC);
}

static void nrf_rf_timeout(EXO* exo)
{
    printk("rf to, state %X\n", exo->rf.state);
    /* timeout occured */
    NVIC_DisableIRQ(RADIO_IRQn);
    /* disable and stop */
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_RADIO->TASKS_STOP = 1;
    /* flush events */
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->EVENTS_READY = 0;

    iio_complete_ex(exo->rf.process,
                    HAL_IO_CMD(HAL_RF, (exo->rf.state == RADIO_STATE_RX) ? IPC_READ : IPC_WRITE),
                    0, exo->rf.io, ERROR_TIMEOUT);

    exo->rf.io = NULL;
    exo->rf.process = INVALID_HANDLE;
    exo->rf.state = RADIO_STATE_IDLE;
}

static void nrf_rf_set_channel(EXO* exo, uint8_t channel)
{
    if(!exo->rf.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    /* Set frequency = 2400 + FREQUENCY (MHz) channel */
    NRF_RADIO->FREQUENCY = channel;
}

static void nrf_rf_set_txpower(EXO* exo, uint8_t power)
{
    if(!exo->rf.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    /* set power */
    NRF_RADIO->TXPOWER = (power << RADIO_TXPOWER_TXPOWER_Pos);
}

#if (0)
static void nrf_rf_advertise_listen(EXO* exo, HANDLE user, IO* io, unsigned int max_size)
{
    RADIO_STACK* stack = io_data(io);
    io_hide(io, sizeof(RADIO_STACK));

    exo->rf.process = user;
    exo->rf.io = io;
    exo->rf.max_size = max_size;

//    if(stack->timeout)
//        ksystime_soft_timer_start_ms(exo->rf.timer, stack->timeout);

   /* Set the pointer to write the incoming packet. */
   NRF_RADIO->PACKETPTR = (uint32_t) exo->rf.pdu;

   memset(exo->rf.pdu, 0xEE, MAX_PDU_SIZE);

   /* Set the initial freq to obsvr as channel 39 */
   // Frequency = 2400 + FREQUENCY (MHz)
   NRF_RADIO->FREQUENCY = BLE_ADV_CHANNEL_39;

   /* Configure the shorts for observing
    * READY event and START task
    * ADDRESS event and RSSISTART task
    * END event and START task
    * */

   NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
                       RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
                       RADIO_SHORTS_END_START_Msk;

   NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk |
                         RADIO_INTENSET_RSSIEND_Msk |
                         RADIO_INTENSET_END_Msk;


//    /* Set the pointer to write the incoming packet. */
   NRF_RADIO->PACKETPTR = (uint32_t) exo->rf.pdu;

   uint32_t temp = NRF_RADIO->FREQUENCY;
   NRF_RADIO->DATAWHITEIV = (temp == 2) ? 37 : ((temp == 26) ? 38 : 39);

   NRF_RADIO->EVENTS_READY = 0;
   NRF_RADIO->TASKS_RXEN = 1;
   NRF_RADIO->TASKS_START = 1;

   kerror(ERROR_SYNC);
}

static void nrf_rf_send_adv(EXO* exo, IO* io)
{
    printk("Send advertising data\n");
    /* Enable power to RADIO */
    NRF_RADIO->POWER = 1;
    /* Set radio transmit power to 0dBm */
    NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);

    /* Set radio mode to 1Mbit/s Bluetooth Low Energy */
    NRF_RADIO->MODE = (RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos);
    // Frequency = 2400 + FREQUENCY (MHz)
    NRF_RADIO->FREQUENCY = BLE_ADV_CHANNEL_39;

    uint32_t temp = NRF_RADIO->FREQUENCY;
    NRF_RADIO->DATAWHITEIV = (temp == 2) ? 37 : ((temp == 26) ? 38 : 39);

    /* Configure Access Address according to the BLE standard */
    NRF_RADIO->PREFIX0 = 0x8e;
    NRF_RADIO->BASE0 = 0x89bed600;

    /* Use logical address 0 (prefix0 + base0) = 0x8E89BED6 when transmitting and receiving */
    NRF_RADIO->TXADDRESS = 0x00;
    NRF_RADIO->RXADDRESSES = 0x01;

    /* PCNF-> Packet Configuration.
     * We now need to configure the sizes S0, S1 and length field to match the
     * datapacket format of the advertisement packets.
     */
    NRF_RADIO->PCNF0 = (
      (((1UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) |  /* Length of S0 field in bytes 0-1.    */
      (((2UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) |  /* Length of S1 field in bits 0-8.     */
      (((6UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk)    /* Length of length field in bits 0-8. */
      );

    /* Packet configuration */
    NRF_RADIO->PCNF1 = (
      (((37UL) << RADIO_PCNF1_MAXLEN_Pos) & RADIO_PCNF1_MAXLEN_Msk)   |                      /* Maximum length of payload in bytes [0-255] */
      (((0UL) << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk)   |                      /* Expand the payload with N bytes in addition to LENGTH [0-255] */
      (((3UL) << RADIO_PCNF1_BALEN_Pos) & RADIO_PCNF1_BALEN_Msk)       |                      /* Base address length in number of bytes. */
      (((RADIO_PCNF1_ENDIAN_Little) << RADIO_PCNF1_ENDIAN_Pos) & RADIO_PCNF1_ENDIAN_Msk) |  /* Endianess of the S0, LENGTH, S1 and PAYLOAD fields. */
      (((1UL) << RADIO_PCNF1_WHITEEN_Pos) & RADIO_PCNF1_WHITEEN_Msk)                         /* Enable packet whitening */
    );

    /* CRC config */
    NRF_RADIO->CRCCNF  = (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos) |
                         (RADIO_CRCCNF_SKIP_ADDR_Skip << RADIO_CRCCNF_SKIP_ADDR_Skip); /* Skip Address when computing CRC */
    NRF_RADIO->CRCINIT = 0x555555;                                                  /* Initial value of CRC */
    NRF_RADIO->CRCPOLY = 0x00065B;                                                  /* CRC polynomial function */

    /* Clear events */
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_ADDRESS = 0;
  //-------------------------------------
  //-------------------------------------
    NRF_RADIO->PACKETPTR = (unsigned int)template_adv_packet;

    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;                                            // Enable radio and wait for ready.
    while (NRF_RADIO->EVENTS_READY == 0U){}

    NRF_RADIO->TASKS_START = 1U;
    NRF_RADIO->EVENTS_END  = 0U;                                        // Start transmission.
    while (NRF_RADIO->EVENTS_END == 0U){}                       // Wait for end of the transmission packet.

    NRF_RADIO->EVENTS_DISABLED = 0U;
    NRF_RADIO->TASKS_DISABLE   = 1U;                                    // Disable the radio.
    while (NRF_RADIO->EVENTS_DISABLED == 0U){}


    /* Set the pointer to write the incoming packet. */
    NRF_RADIO->PACKETPTR = (uint32_t) exo->rf.pdu;

    NRF_RADIO->EVENTS_READY = 0U;                                   //
    NRF_RADIO->TASKS_RXEN   = 1U;                                       // Enable radio.
    while(NRF_RADIO->EVENTS_READY == 0U) {}                     // Wait for an event to be ready.

    NRF_RADIO->EVENTS_END  = 0U;                                //
    NRF_RADIO->TASKS_START = 1U;                                // Start listening and wait for address received event.

    while(NRF_RADIO->EVENTS_END != 0U);

    NRF_RADIO->EVENTS_END = 0U;

    if(NRF_RADIO->CRCSTATUS == 1U)
    {
        //------------------------------------------------------------------------
        //------------------------------------------------------------------------
        if((exo->rf.pdu[0] & 0x0F) == 0x03)
        {
            NRF_RADIO->EVENTS_DISABLED = 0U;
            NRF_RADIO->TASKS_DISABLE   = 1U;                                // Disable the radio.
            while (NRF_RADIO->EVENTS_DISABLED == 0U){}

            //DataOutRSP();
            NRF_RADIO->PACKETPTR = (uint32_t)rsp;

            NRF_RADIO->EVENTS_READY = 0U;
            NRF_RADIO->TASKS_TXEN   = 1;                                            // Enable radio and wait for ready.
            while (NRF_RADIO->EVENTS_READY == 0U){}

            NRF_RADIO->TASKS_START = 1U;
            NRF_RADIO->EVENTS_END  = 0U;                                        // Start transmission.
            while (NRF_RADIO->EVENTS_END == 0U){}                       // Wait for end of the transmission packet.
        }
        //------------------------------------------------------------------------
        //------------------------------------------------------------------------
        if((exo->rf.pdu[0] & 0x0F) == 0x05)
        {
            printk("1\n");
        }
    }

    NRF_RADIO->EVENTS_DISABLED = 0U;
    NRF_RADIO->TASKS_DISABLE   = 1U;            // Disable the radio.
    while (NRF_RADIO->EVENTS_DISABLED == 0U){}
}
#endif //

void nrf_rf_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        nrf_rf_open(exo, ipc->param1);
        break;
    case IPC_CLOSE:
        nrf_rf_close(exo);
        break;
    case IPC_READ:
    case IPC_WRITE:
        nrf_rf_io(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3, (IPC_READ == HAL_ITEM(ipc->cmd)));
        break;
    case IPC_TIMEOUT:
        nrf_rf_timeout(exo);
        break;
    case RADIO_SET_CHANNEL:
        nrf_rf_set_channel(exo, ipc->param1);
        break;
    case RADIO_SET_TXPOWER:
        nrf_rf_set_txpower(exo, ipc->param1);
        break;
//    case BLE_ADVERTISE_LISTEN:
//        nrf_rf_advertise_listen(exo, ipc->process, (IO*)ipc->param2, ipc->param3);
//        break;
//    case BLE_SEND_ADV_DATA:
//        nrf_rf_send_adv(exo, (IO*)ipc->param2);
//        break;
    case RADIO_START:
        nrf_rf_start(exo, ipc->process, (IO*)ipc->param2);
        break;
    case RADIO_STOP:
        nrf_rf_stop(exo);
        break;
    default:
        printk("not supported\n");
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

