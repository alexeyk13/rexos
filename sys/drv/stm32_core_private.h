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
#if (MONOLITH_UART)
#include "stm32_uart.h"
#endif

typedef struct _CORE {
    GPIO_DRV gpio;
    TIMER_DRV timer;
    POWER_DRV power;
#if (MONOLITH_UART)
    UART_DRV uart;
#endif
}CORE;

#if (SYS_INFO)
#if (UART_STDIO) && (MONOLITH_UART)
#define printd          printu
#else
#define printd          printf
#endif
#endif //SYS_INFO


#endif // STM32_CORE_PRIVATE_H
