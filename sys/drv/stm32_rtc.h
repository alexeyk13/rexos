/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_RTC_H
#define STM32_RTC_H

#include "stm32_core.h"
#include "sys_config.h"

time_t stm32_rtc_get();
void stm32_rtc_set(CORE* core, time_t time);

#if (SYS_INFO)
void stm32_rtc_info();
#endif

void stm32_rtc_init();

#endif // STM32_RTC_H
