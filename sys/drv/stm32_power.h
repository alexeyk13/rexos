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
    IPC_UPDATE_CLOCK,
    IPC_GET_RESET_REASON,
    IPC_POWER_BACKUP_ON,                                        //!< Turn on backup domain
    IPC_POWER_BACKUP_OFF,                                       //!< Turn off backup domain
    IPC_POWER_BACKUP_WRITE_ENABLE,                              //!< Turn off write protection of backup domain
    IPC_POWER_BACKUP_WRITE_PROTECT                              //!< Turn on write protection of backup domain
} STM32_POWER_IPCS;

typedef enum {
    STM32_CLOCK_CORE,
    STM32_CLOCK_AHB,
    STM32_CLOCK_APB1,
    STM32_CLOCK_APB2
} STM32_POWER_CLOCKS;

typedef enum {
    RESET_REASON_UNKNOWN    = 0,
    RESET_REASON_LOW_POWER,
    RESET_REASON_WATCHDOG,
    RESET_REASON_SOFTWARE,
    RESET_REASON_POWERON,
    RESET_REASON_PIN_RST
} RESET_REASON;

extern const REX __STM32_POWER;

#endif // STM32_POWER_H
