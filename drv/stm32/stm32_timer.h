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

#include "stm32_core.h"
#include "../../userspace/gpio.h"

typedef enum {
    //timer
    STM32_TIMER_ENABLE = HAL_IPC(HAL_TIMER),
    STM32_TIMER_DISABLE,
    STM32_TIMER_ENABLE_EXT_CLOCK,
    STM32_TIMER_DISABLE_EXT_CLOCK,
    STM32_TIMER_SETUP_HZ,
    STM32_TIMER_START,
    STM32_TIMER_STOP,
    STM32_TIMER_GET_CLOCK,
} STM32_TIMER_IPCS;

typedef struct {
    //timer specific
    int hpet_uspsc;
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    int shared1, shared8;
#endif //defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
}TIMER_DRV;


#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
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
#elif defined(STM32L0)
typedef enum {
    TIM_2 = 0,
    TIM_6,
    TIM_21,
    TIM_22
}TIMER_NUM;
#endif

#define TIMER_FLAG_RISING                            (1 << 0)
#define TIMER_FLAG_FALLING                           (1 << 1)
#define TIMER_FLAG_EDGE_MASK                         (3 << 0)
#define TIMER_FLAG_PULLUP                            (1 << 2)
#define TIMER_FLAG_PULLDOWN                          (1 << 3)
#define TIMER_FLAG_PULL_MASK                         (3 << 2)
#define TIMER_FLAG_ONE_PULSE_MODE                    (1 << 4)
#define TIMER_FLAG_ENABLE_IRQ                        (1 << 5)
#define TIMER_FLAG_PRIORITY                          16

typedef TIM_TypeDef*                            TIM_TypeDef_P;

extern const TIM_TypeDef_P TIMER_REGS[];
extern const int TIMER_VECTORS[];

void stm32_timer_init(CORE* core);
bool stm32_timer_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int stm32_timer_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_timer_request(core, &ipc);
    return ipc.param1;
}


#endif // STM32_TIMER_H
