/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CORE_PRIVATE_H
#define STM32_CORE_PRIVATE_H

#include "stm32_config.h"
#include "stm32_core.h"
#include "stm32_gpio.h"
#include "stm32_timer.h"

typedef struct _CORE {
    GPIO_DRV gpio;
    TIMER_DRV timer;
    //power specific
    int write_count;
    RESET_REASON reset_reason;
#if defined(STM32F1)
    int dma_count[2];
#endif
}CORE;

#endif // STM32_CORE_PRIVATE_H
