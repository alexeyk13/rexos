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
//------------------- RTOS free radio setup -----------------------
static const uint8_t ble_chan_freq[] = {
     4,  6,  8, 10, 12, 14, 16, 18, 20, 22, // 0-9
    24, 28, 30, 32, 34, 36, 38, 40, 42, 44, // 10-19
    46, 48, 50, 52, 54, 56, 58, 60, 62, 64, // 20-29
    66, 68, 70, 72, 74, 76, 78,  2, 26, 80, // 30-39
};

static inline void nrf_apply_errata_102_106_107(void)
{
    /* [102] RADIO: PAYLOAD/END events delayed or not triggered after ADDRESS
     * [106] RADIO: Higher CRC error rates for some access addresses
     * [107] RADIO: Immediate address match for access addresses containing MSBs 0x00
     */
    *(volatile uint32_t *)0x40001774 = ((*(volatile uint32_t *)0x40001774) & 0xfffffffe) | 0x01000000;
}

static void nrf_rf_set_channel_internal(EXO* exo, uint8_t channel)
{
    exo->rf.curr.channel = channel;
    NRF_RADIO->DATAWHITEIV = channel;
    NRF_RADIO->FREQUENCY = ble_chan_freq[channel]; // real frequency = (2400 + FREQUENCY) MHz
}

static inline void nrf_rf_set_txpower(int8_t power)
{
    NRF_RADIO->TXPOWER = (uint32_t)power;
}

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
//    NRF_RADIO->EVENTS_ADDRESS = 0;
//    NRF_RADIO->EVENTS_PAYLOAD = 0;
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
//    NRF_RADIO->EVENTS_DEVMATCH = 0;
//    NRF_RADIO->EVENTS_DEVMISS = 0;
    NRF_RADIO->EVENTS_RSSIEND = 0;
}

static void nrf_rf_disable()
{
    NRF_RADIO->TASKS_DISABLE = 1;
    while(NRF_RADIO->STATE != RADIO_STATE_STATE_Disabled)
    {
        // waiting max 6uS
    }
    nrf_rf_flush_events();
}

static inline void nrf_rf_set_access_addr(uint32_t access_addr)
{
    NRF_RADIO->BASE0 = (access_addr << 8);
    NRF_RADIO->PREFIX0 = (access_addr >> 24);
    nrf_apply_errata_102_106_107();
}

static inline void nrf_rf_setup_ble_mode()
{
    NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

#if defined(NRF51)
    /* copy the BLE override registers from FICR */
    NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
    NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
    NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
    NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
    NRF_RADIO->OVERRIDE4 = NRF_FICR->BLE_1MBIT[4];
#endif // NRF51
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
    NRF_RADIO->PCNF0 |= (1 << RADIO_PCNF0_S0LEN_Pos) | /* 1 byte */
                        (8 << RADIO_PCNF0_LFLEN_Pos) | /* 6 bits */
                        (0 << RADIO_PCNF0_S1LEN_Pos); /* 0 bits */

#if defined(NRF52)
    NRF_RADIO->PCNF0 |= (0 << RADIO_PCNF0_S1INCL_Pos); /* include S1 field only if present*/
    NRF_RADIO->PCNF0 |= (RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos);
#endif // NRF52
    NRF_RADIO->PCNF1 =  (BLE_ADV_PKT_SIZE << RADIO_PCNF1_MAXLEN_Pos) |// Set maximum PAYLOAD size.
            (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos) |// Enable data whitening.
                                      (3UL << RADIO_PCNF1_BALEN_Pos) |// prefix always one byte(LSB) and 3 high byte from BASE0
            (0UL << RADIO_PCNF1_STATLEN_Pos);                         // static length = 0

    NRF_RADIO->TXADDRESS = 0x00000000;  // use Address prefix 0
    NRF_RADIO->RXADDRESSES = 0x00000001;
    nrf_rf_set_access_addr(0x8E89BED6);

    // CRC is 3 bytes long and ignore the access address in the CRC calculation.
    NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos | RADIO_CRCCNF_SKIP_ADDR_Skip << RADIO_CRCCNF_SKIP_ADDR_Pos;
    NRF_RADIO->CRCPOLY = SET_BIT(10) | SET_BIT(9) | SET_BIT(6) | SET_BIT(4) | SET_BIT(3) | SET_BIT(1) | SET_BIT(0);
    NRF_RADIO->CRCINIT = 0x555555UL;
#if defined (NRF52)
    NRF_RADIO->MODECNF0 = (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos) | (RADIO_MODECNF0_DTX_B0 << RADIO_MODECNF0_DTX_Pos);
#endif // NRF52
}

//------------------------- process interrupt ------------------------
static void nrf_rf_init_tx(EXO* exo)
{
    NRF_RADIO->PACKETPTR = (uint32_t) &exo->rf.tx_pdu;
    nrf_rf_set_channel_internal(exo, exo->rf.next.channel);
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
    NRF_RADIO->TASKS_TXEN = 1;
    exo->rf.state = RADIO_STATE_TX;
//    printk("ch:%d h:%x l:%d", exo->rf.curr.channel, exo->rf.tx_pdu.header, exo->rf.tx_pdu.len);
}

static void nrf_rf_init_rx(EXO* exo)
{
    nrf_rf_set_channel_internal(exo, exo->rf.next.channel);
    NRF_RADIO->PACKETPTR = (uint32_t) &exo->rf.rx_buf[exo->rf.rx_head].pkt;
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
    NRF_RADIO->TASKS_RXEN = 1;
    exo->rf.state = RADIO_STATE_RX;
}


bool  __attribute__((weak)) nrf_rf_is_valid_pkt(BLE_ADV_PKT* pkt, void* param)
{
    if(BLE_TYPE(pkt->header) != BLE_TYPE_ADV_NONCONN_IND)
        return false;
    return true;
}

static void nrf_rf_fill_rx_io(BLE_ADV_REC_PDU* rpdu, IO* io)
{
    io_reset(io);
    BLE_ADV_RX_STACK* stack = io_push(io, sizeof(BLE_ADV_RX_STACK));
    memcpy(io_data(io), rpdu->pkt.data, rpdu->pkt.len-6);
    io->data_size = rpdu->pkt.len - 6;
    stack->header = rpdu->pkt.header;
    addrcpy(&stack->mac, &rpdu->pkt.mac);
    stack->ch = rpdu->ch;
    stack->rssi = rpdu->rssi;
//    stack->id =
}

static inline void nrf_rf_irq_end(EXO* exo)
{
    BLE_ADV_REC_PDU* rpdu;
    uint32_t next;
    int8_t rssi;
    NRF_RADIO->EVENTS_END = 0;
    if(exo->rf.state != RADIO_STATE_RX)
        return;
    rpdu = &exo->rf.rx_buf[exo->rf.rx_head];
    if(NRF_RADIO->CRCSTATUS != 1 || (!nrf_rf_is_valid_pkt(&rpdu->pkt, exo->rf.param)) )
    {
        NRF_RADIO->TASKS_START = 1;  // continue receiving
        return;
    }
    rpdu->ch = exo->rf.curr.channel;
    rssi = NRF_RADIO->RSSISAMPLE;
    rpdu->rssi = -rssi;
    if(exo->rf.rx_io)
    {
        nrf_rf_fill_rx_io(rpdu, exo->rf.rx_io);
        iio_complete(exo->rf.process, HAL_IO_CMD(HAL_RF, IPC_READ), 0, exo->rf.rx_io);
        exo->rf.rx_io = NULL;
    }else{
        next = exo->rf.rx_head + 1;
        if (next >= REC_PDU_BUFFER_LEN)
            next = 0;
        if(exo->rf.rx_tail != next)
        {
            exo->rf.rx_head = next;
            NRF_RADIO->PACKETPTR = (uint32_t) &exo->rf.rx_buf[next].pkt;
        }
    }

    NRF_RADIO->TASKS_START = 1;  // continue receiving
}

static inline void nrf_rf_irq_disabled(EXO* exo)
{
    NRF_RADIO->EVENTS_DISABLED = 0;
    switch(exo->rf.state)
    {
    case RADIO_STATE_IDLE:
        break;
    case RADIO_STATE_TX:
        iio_complete(exo->rf.process, HAL_IO_CMD(HAL_RF, IPC_WRITE), exo->rf.tx_handle, exo->rf.tx_io);
        exo->rf.tx_io = NULL;
        if(exo->rf.is_rx)
            nrf_rf_init_rx(exo);
        else
            exo->rf.state = RADIO_STATE_IDLE;
        break;
    case RADIO_STATE_RX:
        if(exo->rf.tx_io != NULL)
            nrf_rf_init_tx(exo);
        else if(exo->rf.is_rx)
            nrf_rf_init_rx(exo);
        else
            exo->rf.state = RADIO_STATE_IDLE;

        break;
    }
}


static inline void nrf_rf_irq(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    if(NRF_RADIO->EVENTS_END)
        nrf_rf_irq_end(exo);

    if(NRF_RADIO->EVENTS_DISABLED)
        nrf_rf_irq_disabled(exo);
}
//------------------------- kernel  ------------------------

static inline void nrf_rf_open(EXO* exo, void* param)
{
    exo->rf.timer = ksystime_soft_timer_create(KERNEL_HANDLE, 0, HAL_RF);
    if(exo->rf.timer == INVALID_HANDLE)
    {
        kerror(ERROR_OUT_OF_MEMORY);
        return;
    }

    exo->rf.param = param;
    NRF_RADIO->POWER = 0;
    NRF_RADIO->POWER = 1;

    nrf_rf_setup_ble_mode();


    kirq_register(KERNEL_HANDLE, RADIO_IRQn, nrf_rf_irq, (void*)exo);
    NVIC_SetPriority(RADIO_IRQn, 11);

   // default
    nrf_rf_set_channel_internal(exo, 37);
    nrf_rf_set_txpower(-20);

    exo->rf.rx_io =exo->rf.tx_io = NULL;
    exo->rf.rx_head = exo->rf.rx_tail = 0;
    exo->rf.state = RADIO_STATE_IDLE;
    nrf_rf_flush_events();
    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk | RADIO_INTENSET_DISABLED_Msk;
    NVIC_EnableIRQ(RADIO_IRQn);

    exo->rf.active = true;
}

static void nrf_rf_close(EXO* exo)
{
    NVIC_DisableIRQ(RADIO_IRQn);
    kirq_unregister(KERNEL_HANDLE, RADIO_IRQn);
    nrf_rf_disable();

    ksystime_soft_timer_stop(exo->rf.timer);
    ksystime_soft_timer_destroy(exo->rf.timer);
    nrf_rf_init(exo);
    NRF_RADIO->POWER = 0;

}

static void nrf_rf_set_channel(EXO* exo, uint8_t channel)
{
    if(channel > 40)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    exo->rf.next.channel = channel;
    switch (exo->rf.state)
    {
    case RADIO_STATE_IDLE:
        nrf_rf_set_channel_internal(exo, channel);
        break;
    case RADIO_STATE_RX:
        exo->rf.next.channel = channel;
        NRF_RADIO->TASKS_DISABLE = 1;
        break;
    case RADIO_STATE_TX:
        break;
    }
}

static void nrf_rf_read(EXO* exo, HANDLE process, IO* io, unsigned int size)
{
    uint32_t tail;
    BLE_ADV_REC_PDU* rpdu;
    if(exo->rf.rx_head != exo->rf.rx_tail)
    {
        tail = exo->rf.rx_tail;
        rpdu = &exo->rf.rx_buf[tail];
        nrf_rf_fill_rx_io(rpdu, io);
        tail++;
        if(tail >= REC_PDU_BUFFER_LEN)
            tail = 0;
        exo->rf.rx_tail = tail;
        kerror(ERROR_OK);
    }else{
        exo->rf.process = process;
        exo->rf.rx_io = io;
        kerror(ERROR_SYNC);
    }

    exo->rf.is_rx = true;
    if(exo->rf.state == RADIO_STATE_IDLE)
        nrf_rf_init_rx(exo);
}

static inline void nrf_rf_write(EXO* exo, HANDLE process, IO* io, uint8_t channel, uint16_t flags, uint32_t id)
{
    BLE_ADV_PKT* tpdu;

    if(exo->rf.tx_io != NULL)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    if(io->data_size > BLE_ADV_DATA_SIZE)
    {
        kerror(ERROR_INVALID_LENGTH);
        return;
    }
    exo->rf.process = process;
    exo->rf.tx_io = io;
    exo->rf.next.channel = channel;

    tpdu = &exo->rf.tx_pdu;
    memcpy(tpdu->data, io_data(io), io->data_size);
    tpdu->len = io->data_size + 6;
    tpdu->header = BLE_TYPE_ADV_NONCONN_IND;
    if(flags & RADIO_FLAG_RANDOM_ADDR)
        tpdu->header |= BLE_TXADD_Msk;
    if(exo->rf.state == RADIO_STATE_IDLE)
        nrf_rf_init_tx(exo);
    else
        NRF_RADIO->TASKS_DISABLE = 1;

    kerror(ERROR_SYNC);

}

static void nrf_rf_timeout(EXO* exo)
{
    return;
    /* stop irq and all current job */
 //   nrf_rf_stop(exo);

    /* send timeout reply back */
    if(exo->rf.state == RADIO_STATE_RX)
    {
        iio_complete_ex(exo->rf.process, HAL_IO_CMD(HAL_RF,IPC_READ), 0, exo->rf.rx_io, ERROR_TIMEOUT);
    }else{
        iio_complete_ex(exo->rf.process, HAL_IO_CMD(HAL_RF,IPC_WRITE), 0, exo->rf.tx_io, ERROR_TIMEOUT);
    }

    /* start irq, flush data */
//    nrf_rf_start(exo);
}

static void nrf_rf_cancel_rx(EXO* exo)
{
    exo->rf.is_rx = false;
    if(exo->rf.state == RADIO_STATE_RX)
    {
        nrf_rf_disable();
        exo->rf.state = RADIO_STATE_IDLE;
    }
    if(exo->rf.rx_io != NULL)
        io_complete_ex(exo->rf.process, HAL_IO_CMD(HAL_RF,IPC_READ), 0, exo->rf.rx_io, ERROR_CONNECTION_CLOSED);

    exo->rf.rx_io = NULL;
    exo->rf.rx_head = exo->rf.rx_tail = 0;
}


static inline void nrf_rf_set_address(EXO* exo, IPC* ipc)
{
    exo->rf.tx_pdu.mac.hi = ipc->param2;
    exo->rf.tx_pdu.mac.lo = ipc->param3;
}

void nrf_rf_init(EXO* exo)
{
    memset(&exo->rf, 0, sizeof(RADIO_DRV));
    exo->rf.active = false;
}

void nrf_rf_request(EXO* exo, IPC* ipc)
{
    if(!exo->rf.active)
    {
        if(HAL_ITEM(ipc->cmd) == IPC_OPEN)
            nrf_rf_open(exo, (void*)ipc->param3);
        else
            kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        kerror(ERROR_ALREADY_CONFIGURED);
        break;
    case IPC_CLOSE:
        nrf_rf_close(exo);
        break;
    case IPC_READ:
        nrf_rf_read(exo, ipc->process, (IO*)ipc->param2, ipc->param3);
        ipc->param3 = ((IO*)ipc->param2)->data_size;
        break;
    case IPC_WRITE:
        exo->rf.tx_handle = ipc->param1;
        nrf_rf_write(exo, ipc->process, (IO*)ipc->param2, ipc->param1 & 0xff, ipc->param1 >> 16, ipc->param3);
        break;
    case IPC_CANCEL_IO:
        nrf_rf_cancel_rx(exo);
        break;
    case IPC_TIMEOUT:
        nrf_rf_timeout(exo);
        break;
    case RADIO_SET_CHANNEL:
        nrf_rf_set_channel(exo, ipc->param1);
        break;
    case RADIO_SET_TXPOWER:
        nrf_rf_set_txpower(ipc->param1);
        break;
    case RADIO_SET_ADV_ADDRESS:
        nrf_rf_set_address(exo, ipc);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

