/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_PIN_H
#define STM32_PIN_H

#include "stm32_exo.h"

typedef struct {
    int* used_pins[GPIO_COUNT];
#if defined(STM32F1)
    int used_afio;
#elif defined(STM32L0)
    int used_syscfg;
#endif
}GPIO_DRV;

void stm32_pin_init(EXO* exo);
void stm32_pin_request(EXO* exo, IPC* ipc);

__STATIC_INLINE unsigned int stm32_pin_request_inside(EXO* exo, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_pin_request(exo, &ipc);
    return ipc.param2;
}


#endif // STM32_PIN_H
