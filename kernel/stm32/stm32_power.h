/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_POWER_H
#define STM32_POWER_H

#include "stm32_exo.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../../userspace/power.h"

typedef struct {
#if (STM32_DECODE_RESET)
    RESET_REASON reset_reason;
#endif //STM32_DECODE_RESET
}POWER_DRV;

void stm32_power_init(EXO* exo);
void stm32_power_request(EXO* exo, IPC* ipc);
int get_core_clock_internal();

#if defined(STM32H7)
void stm32_power_pll2_on();
void stm32_power_pll3_on();
#endif //STM32H7

__STATIC_INLINE unsigned int stm32_power_get_clock_inside(EXO* exo, STM32_POWER_CLOCKS type)
{
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_POWER, POWER_GET_CLOCK);
    ipc.param1 = (unsigned int)type;
    stm32_power_request(exo, &ipc);
    return ipc.param2;
}

#endif // STM32_POWER_H
