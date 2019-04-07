/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#include "nrf_rf.h"
#include "nrf_power.h"
#include "../../userspace/sys.h"
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
#define MAX_PDU_SIZE    40

static inline void print_packet(EXO* exo)
{
    int i = 0;
    uint32_t pkt_size = exo->rf.pdu[1] + 2; // length + header

    for(i = 0; i < pkt_size; i++)
        printk("%02X ", exo->rf.pdu[i]);
    printk("\n");
}

static inline void nrf_rf_irq(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    uint32_t rssi = 0;
    if((NRF_RADIO->EVENTS_END == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk))
    {
        NRF_RADIO->EVENTS_END = 0;
        print_packet(exo);
    }

    if((NRF_RADIO->EVENTS_RSSIEND == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_RSSIEND_Msk))
    {
        NRF_RADIO->EVENTS_RSSIEND = 0;
        NRF_RADIO->TASKS_RSSISTOP = 1;
        rssi = (NRF_RADIO->RSSISAMPLE);
        printk("RSSI: %d\n", rssi);
    }
}

void nrf_rf_init(EXO* exo)
{

}

void nrf_rf_start()
{
    uint32_t temp = NRF_RADIO->FREQUENCY;
    temp = (temp == 2) ? 26 : ((temp == 26) ? 80 : 2);

    NRF_RADIO->FREQUENCY = temp;

    NRF_RADIO->DATAWHITEIV = (temp == 2) ? 37 : ((temp == 26) ? 38 : 39);

    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
}

static inline void nrf_rf_open(EXO* exo)
{
    /* refresh all the registers in RADIO */
    NRF_RADIO->POWER = 0;
    NRF_RADIO->POWER = 1;

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

    /* Set the pointer to write the incoming packet. */
    NRF_RADIO->PACKETPTR = (uint32_t) exo->rf.pdu;

    memset(exo->rf.pdu, 0xEE, MAX_PDU_SIZE);

    /* Set the initial freq to obsvr as channel 37 */
    NRF_RADIO->FREQUENCY = 80;//ADV_CHANNEL[3];

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

    kirq_register(KERNEL_HANDLE, RADIO_IRQn, nrf_rf_irq, (void*)exo);
    // Enable Interrupt for RADIO in the core.
    NVIC_SetPriority(RADIO_IRQn, 2);
    NVIC_EnableIRQ(RADIO_IRQn);

    // temporary
    nrf_rf_start();
}

static inline void nrf_rf_close(EXO* exo)
{

}

void nrf_rf_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        nrf_rf_open(exo);
        break;
    case IPC_CLOSE:
        nrf_rf_close(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
