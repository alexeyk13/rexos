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

void lora_open(HANDLE lora, IO* io_config)
{
    ack(lora, HAL_REQ(HAL_LORA, IPC_OPEN), (unsigned int)io_config, 0, 0);
}

void lora_close(HANDLE lora)
{
    ack(lora, HAL_REQ(HAL_LORA, IPC_CLOSE), 0, 0, 0);
}

void lora_tx_async(HANDLE lora, IO* io)
{
    ack(lora, HAL_REQ(HAL_LORA, IPC_WRITE), (unsigned int)io, 0, 0);
}

void lora_rx_async(HANDLE lora, IO* io)
{
    ack(lora, HAL_REQ(HAL_LORA, IPC_READ), (unsigned int)io, 0, 0);  
}

void lora_get_stats(HANDLE lora, LORA_STATS* stats)
{
    ack(lora, HAL_REQ(HAL_LORA, LORA_GET_STATS), (unsigned int)stats, 0, 0); 
}

void lora_clear_stats(HANDLE lora)
{
    ack(lora, HAL_REQ(HAL_LORA, LORA_CLEAR_STATS), 0, 0, 0); 
}

void lora_abort_rx_transfer(HANDLE lora)
{
    ack(lora, HAL_REQ(HAL_LORA, LORA_ABORT_RX_TRANSFER), 0, 0, 0); 
}
