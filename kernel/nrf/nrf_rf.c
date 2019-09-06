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

/** Return 2^n, used for setting nth bit as 1*/
#define SET_BIT(n)      (1UL << n)

static inline void nrf_rf_flush_events()
{
#if (0)
    printk("R %X\n", NRF_RADIO->EVENTS_READY);
    printk("A %X\n", NRF_RADIO->EVENTS_ADDRESS);
    printk("P %X\n", NRF_RADIO->EVENTS_PAYLOAD);
    printk("E %X\n", NRF_RADIO->EVENTS_END);
    printk("D %X\n", NRF_RADIO->EVENTS_DISABLED);
    printk("M %X\n", NRF_RADIO->EVENTS_DEVMATCH);
    printk("S %X\n", NRF_RADIO->EVENTS_DEVMISS);
    printk("r %X\n", NRF_RADIO->EVENTS_RSSIEND);
#endif //

    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_ADDRESS = 0;
    NRF_RADIO->EVENTS_PAYLOAD = 0;
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->EVENTS_DEVMATCH = 0;
    NRF_RADIO->EVENTS_DEVMISS = 0;
    NRF_RADIO->EVENTS_RSSIEND = 0;
}

static inline void nrf_rf_irq(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    uint8_t pkt_size = 0;
    bool complete = false;
    /* disable timer */
    ksystime_soft_timer_stop(exo->rf.timer);

    /* parse irq */
    switch(exo->rf.state)
    {
        case RADIO_STATE_RX:
        {
            if((NRF_RADIO->EVENTS_RSSIEND == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_RSSIEND_Msk))
            {
                NRF_RADIO->TASKS_RSSISTOP = 1;
                exo->rf.rssi = (NRF_RADIO->RSSISAMPLE);
                exo->rf.state = RADIO_STATE_RX_DATA;
            }

            if((NRF_RADIO->EVENTS_ADDRESS == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_ADDRESS_Msk))
                exo->rf.addr = (NRF_RADIO->RXMATCH);

            break;
        }
        case RADIO_STATE_RX_DATA:
        {
            if(!((NRF_RADIO->EVENTS_END == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)))
                break;

            /* save data size to io */
            pkt_size = exo->rf.pdu[1] + 2;
            /* pkt_size is second byte plus 2 preamble */
            if(exo->rf.io->data_size + pkt_size < io_get_free(exo->rf.io))
            {
                io_data_append(exo->rf.io, exo->rf.pdu, pkt_size);
                io_data_append(exo->rf.io, &(exo->rf.rssi), sizeof(uint32_t));
            }
            complete = true;
            break;
        }

        case RADIO_STATE_TX:
            if(!((NRF_RADIO->EVENTS_END == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)))
                break;

            complete = true;
            break;
        default:
            break;
    }

    if(complete)
    {
        /* disable task */
        NRF_RADIO->TASKS_DISABLE = 1;
        /* send reply back */
        iio_complete(exo->rf.process,
                        HAL_IO_CMD(HAL_RF, (exo->rf.state == RADIO_STATE_TX)? IPC_WRITE : IPC_READ),
                        0, exo->rf.io);

        /* flush data */
        exo->rf.io = NULL;
        exo->rf.process = INVALID_HANDLE;
        exo->rf.state = RADIO_STATE_IDLE;
    }

    nrf_rf_flush_events();
}

void nrf_rf_init(EXO* exo)
{
    exo->rf.active = false;
    exo->rf.timer = INVALID_HANDLE;
    exo->rf.io = NULL;
    exo->rf.process = INVALID_HANDLE;
    exo->rf.max_size = 0;
    exo->rf.state = RADIO_STATE_IDLE;
}

static void nrf_rf_start(EXO* exo)
{
    exo->rf.io = NULL;
    exo->rf.process = INVALID_HANDLE;
    exo->rf.max_size = 0;
    exo->rf.state = RADIO_STATE_IDLE;
    /* disable IRQ */
    NVIC_EnableIRQ(RADIO_IRQn);
}

static void nrf_rf_stop(EXO* exo)
{
    /* disable IRQ */
    NVIC_DisableIRQ(RADIO_IRQn);
    /* disable and stop */
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_RADIO->TASKS_STOP = 1;
    /* flush events */
    nrf_rf_flush_events();
}

static inline void nrf_rf_setup_mode(EXO* exo, RADIO_MODE mode)
{
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
            NRF_RADIO->PREFIX0      = 0xAA55AA55;
            NRF_RADIO->BASE0        = 0x00000000;
            NRF_RADIO->TXADDRESS    = 0x00000000;
            NRF_RADIO->RXADDRESSES  = 0x00000001;

            /* Data whitening */
            NRF_RADIO->DATAWHITEIV = 0x00;

            /* Configure header size. */
            /* S1 size = 0 bits, S0 size = 0 bytes, payload length size = 8 bits */

            NRF_RADIO->PCNF0 |= (8 << RADIO_PCNF0_LFLEN_Pos) |  /* 8 bits */
                                (0 << RADIO_PCNF0_S0LEN_Pos) |  /* 0 byte */
                                (0 << RADIO_PCNF0_S1LEN_Pos);   /* 0 bits */


            /* Set access address to 0x00. This is the access address to be used
            * when send packets in channels. */
           NRF_RADIO->PCNF1   |= 4UL << RADIO_PCNF1_BALEN_Pos;
           /* Enable data whitening. */
           NRF_RADIO->PCNF1 |= RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos;
           /* Set maximum PAYLOAD size. */
           NRF_RADIO->PCNF1 |= MAX_PDU_SIZE << RADIO_PCNF1_MAXLEN_Pos;

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
                                   RADIO_CRCCNF_SKIP_ADDR_Skip << RADIO_CRCCNF_SKIP_ADDR_Pos;

           NRF_RADIO->CRCINIT =    0x555555UL;
           NRF_RADIO->CRCPOLY =    SET_BIT(24) | SET_BIT(10) | SET_BIT(9) |
                                   SET_BIT(6) | SET_BIT(4) | SET_BIT(3) |
                                   SET_BIT(1) | SET_BIT(0);

            break;
        case RADIO_MODE_RF_1Mbit:
            // TODO:
//            printk("not implemented\n");
            kerror(ERROR_NOT_SUPPORTED);
            break;
        case RADIO_MODE_RF_2Mbit:
            // TODO:
//            printk("not implemented\n");
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
            * LENGTH (6 bits)  --> Length(6bits)
            * S0 (1 byte)      --> PDU Type(4bits)|RFU(2bits)|TxAdd(1bit)|RxAdd(1bit)
            * S1 (0 bits)      --> S1(0bits)
            */
           NRF_RADIO->PCNF0 |= (1 << RADIO_PCNF0_S0LEN_Pos) |  /* 1 byte */
                               (8 << RADIO_PCNF0_LFLEN_Pos) |  /* 6 bits */
                               (0 << RADIO_PCNF0_S1LEN_Pos);   /* 0 bits */

            // start observe
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

           /* Enable data whitening. */
           NRF_RADIO->PCNF1 |= RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos;

           /* Set maximum PAYLOAD size. */
           NRF_RADIO->PCNF1 |= MAX_PDU_SIZE << RADIO_PCNF1_MAXLEN_Pos;

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

            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
            return;
    }

       /* Configure the shorts for observing
        * READY event and START task
        * ADDRESS event and RSSISTART task
        * END event and START task
        * */

       NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |
                           RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
                           RADIO_SHORTS_END_START_Msk;
}

static inline void nrf_rf_open(EXO* exo, RADIO_MODE mode)
{
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
    NRF_RADIO->POWER = 0;
    NRF_RADIO->POWER = 1;

    /* Set radio transmit power to default 0dBm */
    NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);

    /* setup mode */
    nrf_rf_setup_mode(exo, mode);

    /* Set the pointer to write the incoming packet. */
    NRF_RADIO->PACKETPTR = (uint32_t) exo->rf.pdu;

    exo->rf.active = true;

    /* setup IRQ */
    kirq_register(KERNEL_HANDLE, RADIO_IRQn, nrf_rf_irq, (void*)exo);
    // Enable Interrupt for RADIO in the core.
    NVIC_SetPriority(RADIO_IRQn, 2);

    nrf_rf_start(exo);
}

static void nrf_rf_close(EXO* exo)
{
    if(!exo->rf.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
            return;
    }
    /* inregister IRQ */
    kirq_unregister(KERNEL_HANDLE, RADIO_IRQn);

    ksystime_soft_timer_destroy(exo->rf.timer);
    nrf_rf_init(exo);
}

static void nrf_rf_io(EXO* exo, HANDLE process, HANDLE user, IO* io, unsigned int size, bool rx)
{
    RADIO_STACK* stack = io_stack(io);
    io_pop(io, sizeof(RADIO_STACK));

    if(!exo->rf.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    exo->rf.io = io;
    exo->rf.max_size = io_get_free(io);
    exo->rf.process = process;

    /* enable interrupt */
    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk |
                          RADIO_INTENSET_RSSIEND_Msk ;

    if(RADIO_MODE_BLE_1Mbit == exo->rf.mode)
    {
        /* set data whiteng for BLE depending on channel */
        NRF_RADIO->DATAWHITEIV = (NRF_RADIO->FREQUENCY == 2) ? 37 : ((NRF_RADIO->FREQUENCY == 26) ? 38 : 39);
    }
    else
    {
        /* RADIO_MODE_RF_1Mbit..2Mbit..250Kbit */
        NRF_RADIO->INTENSET |= RADIO_INTENSET_ADDRESS_Msk;
    }



    /* setup timeout */
    if(stack->flags & RADIO_FLAG_TIMEOUT)
        ksystime_soft_timer_start_ms(exo->rf.timer, stack->timeout_ms);

    /* flush events end */
    nrf_rf_flush_events();

    if(rx)
    {
        /* change state */
        exo->rf.state = RADIO_STATE_RX;
        /* Before the RADIO is able to receive a packet, it must first ramp-up in RX mode */
        NRF_RADIO->TASKS_RXEN = 1;
    }
    else
    {
        /* change state */
        exo->rf.state = RADIO_STATE_TX;
        /* Before the RADIO is able to transmit a packet, it must first ramp-up in TX mode */
        NRF_RADIO->TASKS_TXEN = 1;
    }
    /* Start rx or tx */
    NRF_RADIO->TASKS_START = 1;
    /* wait events in irq */
    kerror(ERROR_SYNC);
}

static void nrf_rf_timeout(EXO* exo)
{
    /* stop irq and all current job */
    nrf_rf_stop(exo);

    /* send timeout reply back */
    iio_complete_ex(exo->rf.process,
                HAL_IO_CMD(HAL_RF, (exo->rf.state == RADIO_STATE_RX) ? IPC_READ : IPC_WRITE),
                0, exo->rf.io, ERROR_TIMEOUT);

    /* start irq, flush data */
    nrf_rf_start(exo);
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

static inline void nrf_rf_set_address(EXO* exo, unsigned int address)
{
    if(!exo->rf.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    /* set address */
    NRF_RADIO->RXADDRESSES = address;
}

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
    case RADIO_SET_ADDRESS:
        nrf_rf_set_address(exo, ipc->param1);
        break;
    case RADIO_START:
        nrf_rf_start(exo);
        break;
    case RADIO_STOP:
        nrf_rf_stop(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

