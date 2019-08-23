/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_radio.h"
#include "nrf_radio_config.h"
#include "sys_config.h"
#include "object.h"
#include "stdio.h"
#include <string.h>

extern void radio_main();

void radio_close()
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, IPC_CLOSE), 0, 0, 0);
}

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
//    HANDLE handle = radio_create(process_name, RADIO_PROCESS_SIZE, RADIO_PROCESS_PRIORITY);
//
//    /* open driver */
//    if(!(INVALID_HANDLE == handle))
//        ack(handle, HAL_REQ(HAL_RF, IPC_OPEN), mode, 0, 0);
//
//    return handle;

    /* straight to KERNEL */

    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, IPC_OPEN), mode, 0, 0);
    // temp
    return INVALID_HANDLE;
}

void radio_start(IO* io)
{
    RADIO_STACK* stack = io_push(io, sizeof(RADIO_STACK));
    io_read_sync(KERNEL_HANDLE, HAL_IO_REQ(HAL_RF, IPC_WRITE), 0, io, 0);
//    io_read_sync(KERNEL_HANDLE, HAL_IO_REQ(HAL_RF, RADIO_START), 0, io, 0);
}

void radio_stop()
{
//    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_STOP), 0, 0, 0);
}

void radio_tx(IO* io, unsigned int timeout_ms)
{

}

void radio_rx()
{

}

static bool radio_tx_sync_internal(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int size, unsigned int timeout_ms, unsigned int flags)
{
    RADIO_STACK* stack = io_push(io, sizeof(RADIO_STACK));
    stack->flags = flags;
    stack->timeout_ms = timeout_ms;
    return (io_read_sync(process, HAL_IO_REQ(hal, IPC_WRITE), user, io, size) == size);
}

bool radio_tx_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int timeout_ms)
{
    unsigned int flags = RADIO_FLAG_EMPTY;
    if(timeout_ms)
        flags |= RADIO_FLAG_TIMEOUT;
    return radio_tx_sync_internal(hal, process, user, io, io->data_size, timeout_ms, flags);
}

bool radio_rx_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int size, unsigned int timeout_ms)
{
    RADIO_STACK* stack = io_push(io, sizeof(RADIO_STACK));
    stack->flags = RADIO_FLAG_EMPTY;
    if(timeout_ms)
        stack->flags |= RADIO_FLAG_TIMEOUT;
    stack->timeout_ms = timeout_ms;
    return (io_read_sync(process, HAL_IO_REQ(hal, IPC_READ), user, io, size) == size);
}

void radio_set_channel(uint8_t channel)
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_SET_CHANNEL), channel, 0, 0);
}

void radio_set_tx_power(uint8_t tx_power_dBm)
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_SET_TXPOWER), tx_power_dBm, 0, 0);
}


