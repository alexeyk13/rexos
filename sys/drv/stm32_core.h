/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CORE_H
#define STM32_CORE_H

#include "stm32_config.h"
#include "../sys.h"
#include "../../userspace/process.h"

typedef enum {
    //timer
    STM32_TIMER_ENABLE = IPC_USER,
    STM32_TIMER_DISABLE,
    STM32_TIMER_START,
    STM32_TIMER_STOP,
    STM32_TIMER_GET_CLOCK,

    //gpio
    STM32_GPIO_ENABLE_PIN,
    STM32_GPIO_ENABLE_PIN_SYSTEM,
    STM32_GPIO_DISABLE_PIN,
    STM32_GPIO_SET_PIN,
    STM32_GPIO_GET_PIN,
    STM32_GPIO_DISABLE_JTAG,

    //power
    STM32_POWER_GET_CLOCK,
    STM32_POWER_UPDATE_CLOCK,
    STM32_POWER_GET_RESET_REASON,
    STM32_POWER_BACKUP_ON,                                        //!< Turn on backup domain
    STM32_POWER_BACKUP_OFF,                                       //!< Turn off backup domain
    STM32_POWER_BACKUP_WRITE_ENABLE,                              //!< Turn off write protection of backup domain
    STM32_POWER_BACKUP_WRITE_PROTECT,                             //!< Turn on write protection of backup domain
    STM32_POWER_ADC_ON,
    STM32_POWER_ADC_OFF,
    STM32_POWER_USB_ON,
    STM32_POWER_USB_OFF,

    STM32_RTC_GET,                                                //!< Get RTC value
    STM32_RTC_SET                                                 //!< Set RTC value

} STM32_CORE_IPCS;

typedef struct {
    //timer specific
    int shared1, shared8;
    int hpet_uspsc;
    //GPIO specific
    int* used_pins;
    //power specific
    int backup_count, write_count;
}CORE;

extern const REX __STM32_CORE;

#endif // STM32_CORE_H
