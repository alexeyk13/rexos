/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_eth.h"
#include "stm32_config.h"
#include "../../userspace/stdio.h"

#define _printd                 printf
#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside
#define ack_power               stm32_core_request_outside

void stm32_eth();

const REX __STM32_ETH = {
    //name
    "STM32 ETH",
    //size
    STM32_ETH_PROCESS_SIZE,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_ETH_IPC_COUNT,
    //function
    stm32_eth
};


void stm32_eth()
{
    for (;;)
    {

    }
}
