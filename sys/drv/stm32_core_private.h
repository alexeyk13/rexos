/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CORE_PRIVATE_H
#define STM32_CORE_PRIVATE_H

#include "stm32_config.h"
#include "stm32_core.h"

typedef struct _CORE {
    //GPIO specific
    int* used_pins[GPIO_COUNT];
#if defined(STM32F1)
    int used_afio;
#elif defined(STM32L0)
    int used_syscfg;
#endif
    //power specific
    int write_count;
    RESET_REASON reset_reason;
#if defined(STM32F1)
    int dma_count[2];
#endif
    //timer specific
    int hpet_uspsc;
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    int shared1, shared8;
#endif //defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
}CORE;

#endif // STM32_CORE_PRIVATE_H
