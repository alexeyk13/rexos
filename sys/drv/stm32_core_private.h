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
#include "stm32_power.h"

typedef struct _CORE {
    GPIO_DRV gpio;
    TIMER_DRV timer;
    POWER_DRV power;
}CORE;

#endif // STM32_CORE_PRIVATE_H
