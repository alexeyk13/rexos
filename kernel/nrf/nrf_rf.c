/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#include "nrf_rf.h"
#include "nrf_power.h"
#include "../../userspace/sys.h"
#include "../../userspace/nrf/radio.h"
#include "../../userspace/nrf/radio_config.h"
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

static inline void print_packet(EXO* exo)
{
    int i = 0;
    uint32_t pkt_size = exo->rf.pdu[1] + 2; // length + header

    for(i = 0; i < pkt_size; i++)
        printk("%02X ", exo->rf.pdu[i]);
    printk("\n");
}

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
    if((NRF_RADIO->EVENTS_END == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk))
    {
        NRF_RADIO->EVENTS_END = 0;
        collect_packet(exo);
    }

    if((NRF_RADIO->EVENTS_RSSIEND == 1) && (NRF_RADIO->INTENSET & RADIO_INTENSET_RSSIEND_Msk))
    {
        NRF_RADIO->EVENTS_RSSIEND = 0;
        NRF_RADIO->TASKS_RSSISTOP = 1;
        exo->rf.rssi = (NRF_RADIO->RSSISAMPLE);
    }
}

void nrf_rf_init(EXO* exo)
{

}

static void adv_seg_print(uint8_t * ptr, uint32_t len){
    uint32_t index = 0;
    while (index < len) {
        uint32_t seg_len = ptr[index];
        index++;

        switch (ptr[index]) {
            case 0: printk("Type Zero is undefined, Error.");
                break;
            case TYPE_FLAGS:
                printk("Flags:");
                break;
            case TYPE_NAME_SHORT:
            case TYPE_NAME:
                printk("Name:");
                break;
            case TYPE_UUID128:
            case TYPE_UUID128_INC:
            case TYPE_UUID32:
            case TYPE_UUID32_INC:
            case TYPE_UUID16:
            case TYPE_UUID16_INC:
                printk("UUIDs:");
                break;
            case TYPE_TRANSMITPOWER:
                printk("Transmit Power:");
                break;
            case TYPE_CONNINTERVAL:
                printk("Connect Interval:");
                break;
            case TYPE_SERVICE_SOLICITATION16:
            case TYPE_SERVICE_SOLICITATION128:
                printk("Service Solicited:");
                break;
            case TYPE_SERVICEDATA:
                printk("Service Data:");
                break;
            case TYPE_MANUFACTURER_SPECIFIC:
                printk("Manufacturer Specific Data:");
                break;
            case TYPE_APPEARANCE:
                printk("Appearance:");
                break;
            default:
                printk("Unknown type:%02X",ptr[index]);
        }

         switch (ptr[index]) {
            case 0: break;
            case TYPE_TRANSMITPOWER:
                printk("%d|",(int)ptr[index+1]);
                break;
            case TYPE_NAME_SHORT:
            case TYPE_NAME:
                printk("%.*s|",(int) seg_len-1,ptr+index+1);
                break;
            case TYPE_SERVICEDATA:
                printk("UUID:0x%02X%02X|",ptr[index+2],ptr[index+1]);
                printk("Data:");
                for(uint32_t i = 3; i<seg_len; i++){
                    printk("%02X ",ptr[index+i]);
                }
                printk("|");
                break;
            case TYPE_UUID128:
            case TYPE_UUID128_INC:
            case TYPE_UUID32:
            case TYPE_UUID32_INC:
            case TYPE_UUID16:
            case TYPE_UUID16_INC:
                printk("UUID:");
                for(uint32_t i = seg_len-1; i>0; i--){
                    printk("%02X",ptr[index+i]);
                }
                printk("|");
                break;
            case TYPE_MANUFACTURER_SPECIFIC:
            case TYPE_CONNINTERVAL:
            /** \todo Display flag information based on the following bits\n
             *  0 LE Limited Discoverable Mode\n
             *  1 LE General Discoverable Mode\n
             *  2 BR/EDR Not Supported\n
             *  3 Simultaneous LE and BR/EDR to Same Device is Capable (Controller)\n
             *  4 Simultaneous LE and BR/EDR to Same Device is Capable (Host)\n
             *  5..7 Reserved
             */
            case TYPE_FLAGS:
            case TYPE_SERVICE_SOLICITATION16:
            case TYPE_SERVICE_SOLICITATION128:
            case TYPE_APPEARANCE:
            default:
                for(uint32_t i = 1; i<seg_len; i++){
                    printk("%02X",ptr[index+i]);
                }
                printk("|");
         }

        index += seg_len;
    }
}

static void print_collected(uint8_t * packet_to_print)
{
    printk("Channel:%d | ", packet_to_print[MAX_PDU_SIZE+4] & 0x7F);
    printk("RSSI:%d|", (int) packet_to_print[MAX_PDU_SIZE+5]);
    printk("CRC:%s|", (packet_to_print[MAX_PDU_SIZE+4]&0x80) ? "Y" : "N");

    /*  0000        ADV_IND
        0001        ADV_DIRECT_IND
        0010        ADV_NONCONN_IND
        0011        SCAN_REQ
        0100        SCAN_RSP
        0101        CONNECT_REQ
        0110        ADV_SCAN_IND
        0111-1111   Reserved    */
//  printk("Type:P%d ",(int) (packet_to_print[0]) & 0xF);
    printk("Type:");
    switch((packet_to_print[0]) & 0xF){
        case TYPE_ADV_IND: printk("ADV_IND|");
                break;
        case TYPE_ADV_DIRECT_IND: printk("ADV_DIRECT_IND|");
                break;
        case TYPE_ADV_NONCONN_IND: printk("ADV_NONCONN_IND|");
                break;
        case TYPE_SCAN_REQ: printk("SCAN_REQ|");
                break;
        case TYPE_SCAN_RSP: printk("SCAN_RSP|");
                break;
        case TYPE_CONNECT_REQ: printk("CONNECT_REQ|");
                break;
        case TYPE_ADV_SCAN_IND: printk("ADV_SCAN_IND|");
                break;
        default:printk("Reserved|");
    }

    /* 1: Random Tx address, 0: Public Tx address */
    printk("TxAdrs:%s|",(int) (packet_to_print[0] & 0x40)?"Random":"Public");
    /* 1: Random Rx address, 0: Public Rx address */
    printk("RxAdrs:%s|",(int) (packet_to_print[0] & 0x80)?"Random":"Public");
    /*  Length of the data packet */
    printk("Len:%d|",(int) packet_to_print[1] & 0x3F);

    uint8_t i;
    uint8_t f = packet_to_print[1] + 2;
    printk("Adrs:");
    for(i = 7; i>2; i--){
        printk("%02X:", packet_to_print[i]);
    }printk("%02X", packet_to_print[2]);
    printk("\n");

    if(packet_to_print[MAX_PDU_SIZE+4]&0x80){
        printk("Packet:");
        for (i = 0; i < f; i++) {
            printk("%02X ", packet_to_print[i]);
        }
        printk("\n");
        uint32_t type_temp = (packet_to_print[0] & 0x7);
        switch(type_temp){
            case TYPE_ADV_IND:
            case TYPE_ADV_NONCONN_IND:
            case TYPE_SCAN_RSP:
            case TYPE_ADV_SCAN_IND:
                adv_seg_print(packet_to_print+8,f-8);
                printk("\n");
                break;
            default: break;
        }
    }else{
        printk("Data corrupted. CRC check failed.\n");
    }
}

static void nrf_rf_start(EXO* exo)
{

}

static void nrf_rf_stop(EXO* exo)
{

}

static inline void nrf_rf_open(EXO* exo)
{
    exo->rf.io = NULL;
    exo->rf.max_size = 0;
    // create timer
    exo->rf.timer = ksystime_soft_timer_create(KERNEL_HANDLE, 0, HAL_RF);

    if(exo->rf.timer == INVALID_HANDLE)
    {
        kerror(ERROR_OUT_OF_MEMORY);
        return;
    }

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

    kirq_register(KERNEL_HANDLE, RADIO_IRQn, nrf_rf_irq, (void*)exo);
    // Enable Interrupt for RADIO in the core.
    NVIC_SetPriority(RADIO_IRQn, 2);
    NVIC_EnableIRQ(RADIO_IRQn);
}

static void nrf_rf_close(EXO* exo)
{

}

static void nrf_rf_timeout(EXO* exo)
{
    // stop
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_STOP = 1;

    iio_complete(exo->rf.process, HAL_IO_CMD(HAL_RF, RADIO_ADVERTISE_LISTEN), 0, exo->rf.io);
    exo->rf.io = NULL;
}

static void nrf_rf_set_channel(EXO* exo, uint8_t channel)
{
    // Set frequency = 2400 + FREQUENCY (MHz) channel
    NRF_RADIO->FREQUENCY = channel;
}

static void nrf_rf_advertise_listen(EXO* exo, HANDLE user, IO* io, unsigned int max_size)
{
    RADIO_STACK* stack = io_data(io);
    io_hide(io, sizeof(RADIO_STACK));

    exo->rf.process = user;
    exo->rf.io = io;
    exo->rf.max_size = max_size;

    if(stack->timeout)
        ksystime_soft_timer_start_ms(exo->rf.timer, stack->timeout);

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

   /* Set the initial freq to obsvr as channel 39 */
   // Frequency = 2400 + FREQUENCY (MHz)
   NRF_RADIO->FREQUENCY = RADIO_BLE_ADV_CHANNEL_39;

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
    case IPC_TIMEOUT:
        nrf_rf_timeout(exo);
        break;
    case RADIO_SET_CHANNEL:
        nrf_rf_set_channel(exo, ipc->param1);
        break;
    case RADIO_ADVERTISE_LISTEN:
        nrf_rf_advertise_listen(exo, ipc->process, (IO*)ipc->param2, ipc->param3);
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
