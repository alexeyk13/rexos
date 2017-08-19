/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_RTC_H
#define STM32_RTC_H

#include "stm32_exo.h"

void stm32_rtc_init();
void stm32_rtc_request(IPC* ipc);

void stm32_rtc_disable();

#endif // STM32_RTC_H
