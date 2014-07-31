/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_TIMER_H
#define STM32_TIMER_H

/*
        timer for STM32
  */

#include "sys_config.h"
#include "stm32_core.h"

typedef enum {
    TIM_1 = 0,
    TIM_2,
    TIM_3,
    TIM_4,
    TIM_5,
    TIM_6,
    TIM_7,
    TIM_8,
    TIM_9,
    TIM_10,
    TIM_11,
    TIM_12,
    TIM_13,
    TIM_14,
    TIM_15,
    TIM_16,
    TIM_17,
    TIM_18,
    TIM_19,
    TIM_20
}TIMER_NUM;

#define TIMER_FLAG_ONE_PULSE_MODE                (1 << 0)

void stm32_timer_enable(CORE* core, TIMER_NUM num, unsigned int flags);
void stm32_timer_disable(CORE* core, TIMER_NUM num);
void stm32_timer_start(TIMER_NUM num, unsigned int psc, unsigned int count);
void stm32_timer_stop(TIMER_NUM num);
unsigned int stm32_timer_get_clock(TIMER_NUM num);
#if (SYS_INFO)
void stm32_timer_info();
#endif
void stm32_timer_init(CORE* core);


#endif // STM32_TIMER_H
