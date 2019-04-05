/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef RTC_H
#define RTC_H

#include "sys.h"
#include "time.h"
#include "ipc.h"

#define RTC_HANDLE_DEVICE                                   0xffff

typedef enum {
    RTC_GET = IPC_USER,                                     //!< Get RTC value
    RTC_SET,                                                //!< Set RTC valu
    RTC_SET_ALARM_SEC,                                      //
    RTC_HAL_MAX
} RTC_IPCS;

TIME* rtc_get(TIME* time);
void rtc_set(TIME* time);

#endif // RTC_H
