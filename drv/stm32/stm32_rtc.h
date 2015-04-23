/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_RTC_H
#define STM32_RTC_H

#include "stm32_core.h"
#include "stm32_config.h"

void stm32_rtc_init();
bool stm32_rtc_request(IPC* ipc);

#if (POWER_DOWN_SUPPORT)
void stm32_rtc_disable();
#endif //POWER_DOWN_SUPPORT

#endif // STM32_RTC_H
