/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_GPIO_H
#define STM32_GPIO_H

#include "stm32_core.h"

typedef struct {
    int* used_pins[GPIO_COUNT];
#if defined(STM32F1)
    int used_afio;
#elif defined(STM32L0)
    int used_syscfg;
#endif
}GPIO_DRV;

void stm32_gpio_init(CORE* core);
bool stm32_gpio_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int stm32_gpio_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_gpio_request(core, &ipc);
    return ipc.param2;
}


#endif // STM32_GPIO_H
