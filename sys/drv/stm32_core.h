/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CORE_H
#define STM32_CORE_H

#include "stm32_config.h"
#include "sys_config.h"
#include "../sys.h"
#include "../../userspace/process.h"

typedef enum {
    //timer
    STM32_TIMER_ENABLE = IPC_USER,
    STM32_TIMER_DISABLE,
    STM32_TIMER_ENABLE_EXT_CLOCK,
    STM32_TIMER_DISABLE_EXT_CLOCK,
    STM32_TIMER_SETUP_HZ,
    STM32_TIMER_START,
    STM32_TIMER_STOP,
    STM32_TIMER_GET_CLOCK,

    //gpio
    STM32_GPIO_ENABLE_PIN,
    STM32_GPIO_ENABLE_PIN_SYSTEM,
    STM32_GPIO_DISABLE_PIN,
    STM32_GPIO_ENABLE_EXTI,
    STM32_GPIO_DISABLE_EXTI,
    STM32_GPIO_SET_PIN,
    STM32_GPIO_GET_PIN,
    STM32_GPIO_DISABLE_JTAG,

    //power
    STM32_POWER_GET_CLOCK,
    STM32_POWER_UPDATE_CLOCK,
    STM32_POWER_GET_RESET_REASON,
    STM32_POWER_DMA_ON,
    STM32_POWER_DMA_OFF,
    STM32_POWER_USB_ON,
    STM32_POWER_USB_OFF,

    //RTC
    STM32_RTC_GET,                                                //!< Get RTC value
    STM32_RTC_SET,                                                //!< Set RTC value

    STM32_WDT_KICK

} STM32_CORE_IPCS;

typedef enum {
    RESET_REASON_UNKNOWN    = 0,
    RESET_REASON_LOW_POWER,
    RESET_REASON_WATCHDOG,
    RESET_REASON_SOFTWARE,
    RESET_REASON_POWERON,
    RESET_REASON_PIN_RST
} RESET_REASON;

typedef struct {
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

extern const REX __STM32_CORE;

#endif // STM32_CORE_H
