/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_RTC_H
#define STM32_RTC_H

#include "stm32_core.h"

void stm32_rtc_init();
bool stm32_rtc_request(IPC* ipc);

#endif // STM32_RTC_H
