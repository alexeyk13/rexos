/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "sys_config.h"
#include "object.h"
#include "nrf_ble.h"
#include "stdio.h"
#include "io.h"
#include <string.h>

#if (BLE_DEBUG_ADV_DATA)
static void ble_debug_adv_data(uint8_t* data, unsigned int size)
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
static void ble_debug_adv_common(IO* io)
{
    unsigned int size = 0;
    uint8_t *p = io_data(io);
    RADIO_BLE_PKT_HEADER* pkt;

#if (BLE_DEBUG_ADV_RAW) // PRINT RAW DATA
    printf("ADV Channel Raw Data: \n");
    for(int i = 0; i < io->data_size; i++)
        printf("%02X ", ((uint8_t*)(io_data(io)))[i]);
    printf("\n");
#endif // BLE_DEBUG_ADV_RAW
    // parse data in IO
#if (BLE_DEBUG_ADV_COMMON)
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
#if (BLE_DEBUG_ADV_DATA)
        // Packet data
        if(pkt->length > 6)
            ble_debug_adv_data(&pkt->data, pkt->length - 6 - 1);
#endif // BLE_DEBUG_ADV_DATA
    }
#endif // BLE_DEBUG_ADV_COMMON
}
#endif // BLE_DEBUG_ADV_DATA

HANDLE ble_open()
{
    return radio_open("BLE stack", RADIO_MODE_BLE_1Mbit);
}


void ble_close()
{
    radio_close();
}
bool ble_send_adv(uint8_t channel, uint8_t* adv_data, unsigned int data_size)
{
    IO* io = io_create(data_size);
    if(io == NULL)
        return false;
    io_write_sync_exo(HAL_IO_REQ(HAL_RF, BLE_SEND_ADV_DATA), 0, io);
    io_destroy(io);
    return true;
}

bool ble_listen_adv_channel(unsigned int max_size, uint8_t flags, unsigned int timeout_ms)
{
    // Create IO
    IO* io = io_create(sizeof(RADIO_STACK) + max_size);
    if(io == NULL)
        return false;
    RADIO_STACK* stack = io_data(io);
    stack->flags = flags;
    stack->timeout = timeout_ms;
    // send IO to receive data
    io_read_sync_exo(HAL_IO_REQ(HAL_RF, BLE_ADVERTISE_LISTEN), 0, io, max_size);
    // print data
#if (BLE_DEBUG_ADV_COMMON)
    ble_debug_adv_common(io);
#endif // RADIO_DEBUG_ADV_COMMON
    // destory IO
    io_destroy(io);
    return true;
}
