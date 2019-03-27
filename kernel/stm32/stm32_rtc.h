/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/
#ifndef STM32_RTC_H
#define STM32_RTC_H

#include "stm32_exo.h"

void stm32_rtc_init();
void stm32_rtc_request(IPC* ipc);

void stm32_rtc_disable();

#endif // STM32_RTC_H
