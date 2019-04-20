/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#include "sys_config.h"
#include "object.h"
#include "lora.h"
#include "io.h"
#include <string.h>

extern void loras_main();

HANDLE lora_create(uint32_t process_size, uint32_t priority)
{
    char name[] = "LoRa Server";
    REX rex;
    rex.name = name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = loras_main;
    return process_create(&rex);
}

int lora_open(HANDLE lora, IO* io_config)
{
    return get_size(lora, HAL_REQ(HAL_LORA, IPC_OPEN), (unsigned int)io_config, 0, 0);
}

void lora_close(HANDLE lora)
{
    ack(lora, HAL_REQ(HAL_LORA, IPC_CLOSE), 0, 0, 0);
}

void lora_tx_async(HANDLE lora, IO* io)
{
    if (lora == INVALID_HANDLE)
        return;
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_LORA, IPC_WRITE);
    ipc.process = lora;
    ipc.param1 = (unsigned int)io;
    ipc.param2 = 0;
    ipc.param3 = 0;
    ipc_post(&ipc);
}

void lora_rx_async(HANDLE lora, IO* io)
{
    if (lora == INVALID_HANDLE)
        return;
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_LORA, IPC_READ);
    ipc.process = lora;
    ipc.param1 = (unsigned int)io;
    ipc.param2 = 0;
    ipc.param3 = 0;
    ipc_post(&ipc);
}

void lora_get_stats(HANDLE lora, IO* io_stats_tx, IO* io_stats_rx)
{
    ack(lora, HAL_REQ(HAL_LORA, LORA_GET_STATS), (unsigned int)io_stats_tx, (unsigned int)io_stats_rx, 0);
}

void lora_clear_stats(HANDLE lora)
{
    ack(lora, HAL_REQ(HAL_LORA, LORA_CLEAR_STATS), 0, 0, 0);
}

void lora_abort_rx_transfer(HANDLE lora)
{
    ack(lora, HAL_REQ(HAL_LORA, IPC_CANCEL_IO), 0, 0, 0);
}
