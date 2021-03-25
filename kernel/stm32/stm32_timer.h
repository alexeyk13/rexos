/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_TIMER_H
#define STM32_TIMER_H

/*
        timer for STM32
  */

#include "stm32_exo.h"
#include "../../userspace/gpio.h"
#include "../../userspace/htimer.h"

typedef struct {
    //timer specific
    int hpet_uspsc;
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4) || defined(STM32H7)
    int shared1, shared8;
#endif //defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
} TIMER_DRV;

void stm32_timer_init(EXO* exo);
void stm32_timer_request(EXO* exo, IPC* ipc);
void stm32_timer_adjust(EXO* exo);

#if (POWER_MANAGEMENT)
void stm32_timer_pm_event(EXO* exo);
#endif //POWER_MANAGEMENT

__STATIC_INLINE unsigned int stm32_timer_request_inside(EXO* exo, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_timer_request(exo, &ipc);
    return ipc.param2;
}

#endif // STM32_TIMER_H
