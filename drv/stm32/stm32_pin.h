/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_PIN_H
#define STM32_PIN_H

#include "stm32_core.h"

typedef struct {
    int* used_pins[GPIO_COUNT];
#if defined(STM32F1)
    int used_afio;
#elif defined(STM32L0)
    int used_syscfg;
#endif
}GPIO_DRV;

void stm32_pin_init(CORE* core);
void stm32_pin_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int stm32_pin_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_pin_request(core, &ipc);
    return ipc.param2;
}


#endif // STM32_PIN_H
