/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "sys_config.h"
#include "object.h"
#include "radio.h"
#include "stdio.h"
#include "io.h"
#include <string.h>

extern void radio_main();

void radio_open(RADIO_MODE mode)
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, IPC_OPEN), mode, 0, 0);
}

void radio_close()
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, IPC_CLOSE), 0, 0, 0);
}

HANDLE radio_create(uint32_t process_size, uint32_t priority)
{
    char name[] = "RF";
    REX rex;
    rex.name = name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = radio_main;
    return process_create(&rex);
}

void radio_start()
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_START), 0, 0, 0);
}

void radio_stop()
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_STOP), 0, 0, 0);
}

void radio_tx()
{

}

void radio_rx()
{

}

void radio_tx_sync()
{

}

void radio_rx_sync()
{

}

bool radio_get_advertise(unsigned int max_size, uint8_t flags, unsigned int timeout_ms)
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

#if (1) // PRINT RAW DATA
    printf("ADV Channel: \n");
    for(int i = 0; i < io->data_size; i++)
        printf("%02X ", ((uint8_t*)(io_data(io)))[i]);
    printf("\n");
#endif
    // parse data in IO
#if (1) // PRINT PACKET DATA
    uint8_t *p = io_data(io);
    // First byte - type
    printf("type: %d, ", *p++);
    printf("size: %d, ", *p++);

    printf("addr: ");
    for(uint8_t i = 0; i < 5; i++)
        printf("%02X:", *(p + 5 - i));
    printf("%02X\n", *p);
#endif
    // destory IO
    io_destroy(io);
    return true;
}
