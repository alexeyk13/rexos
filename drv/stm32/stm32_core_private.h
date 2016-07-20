/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CORE_PRIVATE_H
#define STM32_CORE_PRIVATE_H

#include "stm32_config.h"
#include "stm32_core.h"
#include "stm32_pin.h"
#include "stm32_timer.h"
#include "stm32_power.h"
#include "stm32_uart.h"
#include "stm32_dac.h"
#include "stm32_adc.h"
#include "stm32_i2c.h"
#ifdef STM32F10X_CL
#include "stm32_otg.h"
#else
#include "stm32_usb.h"
#endif

typedef struct _CORE {
    GPIO_DRV gpio;
    TIMER_DRV timer;
    POWER_DRV power;
#if (STM32_UART_DRIVER)
    UART_DRV uart;
#endif //STM32_UART_DRIVER
#if (STM32_ADC_DRIVER)
    ADC_DRV adc;
#endif //STM32_ADC_DRIVER
#if (STM32_DAC_DRIVER)
    DAC_DRV dac;
#endif //STM32_DAC_DRIVER
#if (STM32_I2C_DRIVER)
    I2C_DRV i2c;
#endif //STM32_I2C_DRIVER
#if (STM32_USB_DRIVER)
    USB_DRV usb;
#endif //STM32_USB_DRIVER
}CORE;

#endif // STM32_CORE_PRIVATE_H
