/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RTC_H
#define RTC_H

#include "sys.h"
#include "time.h"
#include "ipc.h"

typedef enum {
    RTC_GET = IPC_USER,                                     //!< Get RTC value
    RTC_SET,                                                //!< Set RTC value

    RTC_HAL_MAX
} RTC_IPCS;

TIME* rtc_get(TIME* time);
void rtc_set(TIME* time);

#endif // RTC_H
