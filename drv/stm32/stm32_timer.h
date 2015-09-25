/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_TIMER_H
#define STM32_TIMER_H

/*
        timer for STM32
  */

#include "stm32_core.h"
#include "../../userspace/gpio.h"
#include "../../userspace/htimer.h"

typedef struct {
    //timer specific
    int hpet_uspsc;
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    int shared1, shared8;
#endif //defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
} TIMER_DRV;

void stm32_timer_init(CORE* core);
void stm32_timer_request(CORE* core, IPC* ipc);

#if (POWER_MANAGEMENT_SUPPORT)
void stm32_timer_pm_event(CORE* core);
#endif //POWER_MANAGEMENT_SUPPORT

__STATIC_INLINE unsigned int stm32_timer_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_timer_request(core, &ipc);
    return ipc.param2;
}

#endif // STM32_TIMER_H
