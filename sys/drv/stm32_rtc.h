/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_RTC_H
#define STM32_RTC_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"

typedef enum {
    IPC_RTC_GET = IPC_USER,                                            //!< Get RTC value
    IPC_RTC_SET                                                        //!< Set RTC value
} STM32_RTC_IPCS;


extern const REX __STM32_RTC;

#endif // STM32_RTC_H
