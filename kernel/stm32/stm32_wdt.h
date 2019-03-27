/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_WDT_H
#define STM32_WDT_H

#include "../../userspace/sys.h"

void stm32_wdt_pre_init();
void stm32_wdt_init();

void stm32_wdt_request(IPC* ipc);


#endif // STM32_WDT_H
