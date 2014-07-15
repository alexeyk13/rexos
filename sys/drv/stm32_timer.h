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

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"

typedef enum {
    IPC_TIMER_ENABLE = IPC_USER,
    IPC_TIMER_DISABLE,
    IPC_TIMER_START,
    IPC_TIMER_STOP,
    IPC_TIMER_GET_CLOCK
} STM32_TIMER_IPCS;

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

extern const REX __STM32_TIMER;

#endif // STM32_TIMER_H
