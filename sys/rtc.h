/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RTC_H
#define RTC_H

#include "sys.h"
#include "sys_config.h"
#include "../userspace/cc_macro.h"
#include "../userspace/ipc.h"

typedef enum {
    RTC_GET = HAL_IPC(HAL_RTC),                             //!< Get RTC value
    RTC_SET,                                                //!< Set RTC value

    GPIO_RTC_MAX
} RTC_IPCS;

#endif // RTC_H
