/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "sys_config.h"
#include "object.h"
#include "radio.h"
#include "radio_config.h"
#include "stdio.h"
#include "io.h"
#include <string.h>


#if (RADIO_DEBUG_ADV_DATA)
static void radio_debug_adv_data(uint8_t* data, unsigned int size)
{
    unsigned int dispatched = 0;
    RADIO_BLE_LTV* p = (RADIO_BLE_LTV*)data;
    printf("DATA (%d):\n", size);
    for(uint8_t i = 0; i < size; i++)
    {
        printf("%02X ", *((uint8_t*)(data + i)));
        if((i % 0x10) == 0xf)
            printf("\n");
    }
    printf("\n");

    while(1)
    {
        p = (RADIO_BLE_LTV*)(data + dispatched);
        printf("Len:  %-2.2d,", p->length);
        printf("Type: %02X\n", p->type);
        dispatched += p->length + sizeof(uint8_t);

        if(dispatched > size)
            break;
    }

    printf("========================\n");

}
static void radio_debug_adv_common(IO* io)
{
    unsigned int size = 0;
    uint8_t *p = io_data(io);
    RADIO_BLE_PKT_HEADER* pkt;

#if (RADIO_DEBUG_ADV_RAW) // PRINT RAW DATA
    printf("ADV Channel Raw Data: \n");
    for(int i = 0; i < io->data_size; i++)
        printf("%02X ", ((uint8_t*)(io_data(io)))[i]);
    printf("\n");
#endif // RADIO_DEBUG_ADV_RAW
    // parse data in IO
#if (RADIO_DEBUG_ADV_COMMON)
    printf("ADV Channel Captured \n");
    while(size < io->data_size)
    {
        // First byte - type
        pkt = (RADIO_BLE_PKT_HEADER*)(p + size);
        printf("type: %02Xh, ", pkt->type);

        switch(BLE_TYPE_Msk(pkt->type))
        {
            case BLE_TYPE_ADV_IND:        printf("        ADV, ");   break;
            case BLE_TYPE_ADV_DIRECT_IND: printf(" ADV_DIRECT, ");   break;
            case BLE_TYPE_ADV_NONCONN_IND:printf("    NONCONN, ");   break;
            case BLE_TYPE_SCAN_REQ:       printf("   SCAN_REQ, ");   break;
            case BLE_TYPE_SCAN_RSP:       printf("   SCAN_RSP, ");   break;
            case BLE_TYPE_CONNECT_REQ:    printf("CONNECT_REQ, ");   break;
            case BLE_TYPE_ADV_SCAN_IND:   printf("       SCAN, ");   break;
            default:                      printf("    UNKNOWN, ");   break;
        }

        // Pkt length
        printf("len: %-3.3d, ", pkt->length);
        // Addr in reverse bytes
        printf("addr: ");
        for(uint8_t i = 1; i < 6; i++)
            printf("%02X:", pkt->addr[RADIO_BLE_PKT_ADDR_SIZE - i]);
        printf("%02X, ", pkt->addr[0]);
        // size =  sizeof(header) + pkt length
        size += 2 + pkt->length;
        // RSSI in the end of packet
        printf("RSSI: %d dBm\n", p[size]);
        size += sizeof(uint32_t);
#if (RADIO_DEBUG_ADV_DATA)
        // Packet data
        if(pkt->length > 6)
            radio_debug_adv_data(&pkt->data, pkt->length - 6 - 1);
#endif // RADIO_DEBUG_ADV_DATA
    }
#endif // RADIO_DEBUG_ADV_COMMON
}

#endif // RADIO_DEBUG_ADV_DATA

extern void radio_main();

//void radio_close()
//{
//    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, IPC_CLOSE), 0, 0, 0);
//}

HANDLE radio_create(char* process_name, uint32_t process_size, uint32_t priority)
{
    REX rex;
    rex.name = process_name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = radio_main;
    return process_create(&rex);
}

HANDLE radio_open(char* process_name, RADIO_MODE mode)
{
    HANDLE handle = radio_create(process_name, RADIO_PROCESS_SIZE, RADIO_PROCESS_PRIORITY);
    if(INVALID_HANDLE == handle)
        return handle;

    ack(handle, HAL_REQ(HAL_RF, IPC_OPEN), mode, 0, 0);
    return handle;
}

HANDLE ble_open()
{
    return radio_open("BLE stack", RADIO_MODE_BLE_1Mbit);
}

//void radio_start()
//{
//    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_START), 0, 0, 0);
//}
//
//void radio_stop()
//{
//    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_STOP), 0, 0, 0);
//}
//
//void radio_tx()
//{
//
//}
//
//void radio_rx()
//{
//
//}
//void radio_tx_sync()
//{
//
//}
//
//void radio_rx_sync()
//{
//
//}
//
//void radio_set_channel(uint8_t channel)
//{
//    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_SET_CHANNEL), channel, 0, 0);
//}

bool radio_send_adv(uint8_t channel, uint8_t* adv_data, unsigned int data_size)
{
    IO* io = io_create(data_size);
    if(io == NULL)
        return false;
    io_write_sync_exo(HAL_IO_REQ(HAL_RF, RADIO_SEND_ADV_DATA), 0, io);
    io_destroy(io);
    return true;
}

bool radio_listen_adv_channel(unsigned int max_size, uint8_t flags, unsigned int timeout_ms)
{
    // Create IO
    IO* io = io_create(sizeof(RADIO_STACK) + max_size);
    if(io == NULL)
        return false;
    RADIO_STACK* stack = io_data(io);
    stack->flags = flags;
    stack->timeout = timeout_ms;
    // send IO to receive data
    io_read_sync_exo(HAL_IO_REQ(HAL_RF, RADIO_ADVERTISE_LISTEN), 0, io, max_size);
    // print data
#if (RADIO_DEBUG_ADV_COMMON)
    radio_debug_adv_common(io);
#endif // RADIO_DEBUG_ADV_COMMON
    // destory IO
    io_destroy(io);
    return true;
}

