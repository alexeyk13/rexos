/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CORE_PRIVATE_H
#define STM32_CORE_PRIVATE_H

#include "stm32_config.h"
#include "stm32_core.h"
#include "stm32_gpio.h"
#include "stm32_timer.h"
#include "stm32_power.h"
#include "stm32_uart.h"
#include "stm32_dac.h"
#include "stm32_adc.h"
#include "stm32_eep.h"
#ifdef STM32L0
#include "stm32_usb.h"
#else
#include "stm32_otg.h"
#endif

typedef struct _CORE {
    GPIO_DRV gpio;
    TIMER_DRV timer;
    POWER_DRV power;
#if (MONOLITH_UART)
    UART_DRV uart;
#endif
#if (STM32_ADC_DRIVER)
    ADC_DRV adc;
#endif //STM32_ADC_DRIVER
#if (STM32_DAC_DRIVER)
    DAC_DRV dac;
#endif //STM32_DAC_DRIVER
#if (STM32_EEP_DRIVER)
    EEP eep;
#endif //STM32_EEP_DRIVER
#if (MONOLITH_USB)
    USB_DRV usb;
#endif
}CORE;

#endif // STM32_CORE_PRIVATE_H
