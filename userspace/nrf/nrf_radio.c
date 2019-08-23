/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "sys_config.h"
#include "object.h"
#include "nrf_radio.h"
#include "nrf_radio_config.h"
#include "stdio.h"
#include "io.h"
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
    HANDLE handle = radio_create(process_name, RADIO_PROCESS_SIZE, RADIO_PROCESS_PRIORITY);
    if(INVALID_HANDLE == handle)
        return handle;

    ack(handle, HAL_REQ(HAL_RF, IPC_OPEN), mode, 0, 0);
    return handle;
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
