/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_POWER_H
#define STM32_POWER_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"

//special values for STM32F1_CL
#define PLL_MUL_6_5                             15
#define PLL2_MUL_20                             15

typedef enum {
    IPC_GET_CLOCK = IPC_USER,
    IPC_SET_CORE_CLOCK,
    IPC_POWER_TEST
} STM32_POWER_IPCS;

typedef enum {
    STM32_CLOCK_CORE,
    STM32_CLOCK_AHB,
    STM32_CLOCK_APB1,
    STM32_CLOCK_APB2
} STM32_POWER_CLOCKS;

typedef enum {
    RESET_REASON_UNKNOWN    = 0,
    RESET_REASON_STANBY,
    RESET_REASON_WATCHDOG,
    RESET_REASON_SOFTWARE,
    RESET_REASON_POWERON,
    RESET_REASON_PIN_RST
} RESET_REASON;

RESET_REASON get_reset_reason();

extern const REX __STM32_POWER;

#endif // STM32_POWER_H
