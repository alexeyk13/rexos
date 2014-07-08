/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARCH_STM32_H
#define ARCH_STM32_H

//remove this shit later

#include "stm32.h"

//only gpio pin related
#define EXTI_LINES_COUNT                    16

#define UARTS_COUNT                            5

#define TIMERS_MASK    (1 << TIM_1)  || (1 << TIM_2)  || (1 << TIM_3)  || (1 << TIM_4) || (1 << TIM_5) || (1 << TIM_6) || (1 << TIM_7)
#define TIMERS_COUNT                            7
#define GPIO_PORTS_COUNT                    7

#define MAX_FS_FREQ                            48000000l

#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
#define _STM
#endif

#endif //ARCH_STM32_H
