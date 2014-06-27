/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RTC_H
#define RTC_H

#include "dev.h"
#include "time.h"

typedef void (*RTC_HANDLER)(RTC_CLASS);

extern void rtc_enable(RTC_CLASS rtc);
extern void rtc_disable(RTC_CLASS rtc);
extern void rtc_enable_second_tick(RTC_CLASS rtc, RTC_HANDLER callback, int priority);
extern void rtc_disable_second_tick(RTC_CLASS rtc);

extern time_t rtc_get(RTC_CLASS rtc);
extern void rtc_set(RTC_CLASS rtc, time_t time);

#endif // RTC_H
