/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_EXO_PRIVATE_H
#define STM32_EXO_PRIVATE_H

#include "stm32_config.h"
#include "stm32_exo.h"
#include "stm32_pin.h"
#include "stm32_timer.h"
#include "stm32_power.h"
#include "stm32_uart.h"
#include "stm32_can.h"
#include "stm32_dac.h"
#include "stm32_adc.h"
#include "stm32_flash.h"
#include "stm32_eth.h"
#include "stm32_sdmmc.h"
#include "stm32_rng.h"
#if (STM32_I2C_DRIVER)
    #include "stm32_i2c.h"
#endif //STM32_I2C_DRIVER
#if (STM32_SPI_DRIVER)
    #include "stm32_spi.h"
#endif //STM32_SPI_DRIVER
#if defined(STM32F10X_CL) || defined(STM32H7)
#include "stm32_otg.h"
#else
#include "stm32_usb.h"
#endif

typedef struct _EXO {
    GPIO_DRV gpio;
    TIMER_DRV timer;
    POWER_DRV power;
#if (STM32_UART_DRIVER)
    UART_DRV uart;
#endif //STM32_UART_DRIVER
#if (STM32_CAN_DRIVER)
    CAN_DRV can;
#endif //STM32_CAN_DRIVER
#if (STM32_SPI_DRIVER)
    SPI_DRV spi;
#endif //STM32_SPI_DRIVER
#if (STM32_ADC_DRIVER)
    ADC_DRV adc;
#endif //STM32_ADC_DRIVER
#if (STM32_DAC_DRIVER)
    DAC_DRV dac;
#endif //STM32_DAC_DRIVER
#if (STM32_I2C_DRIVER)
    I2C_DRV i2c;
#endif //STM32_I2C_DRIVER
#if (STM32_FLASH_DRIVER)
    FLASH_DRV flash;
#endif //STM32_FLASH_DRIVER
#if (STM32_USB_DRIVER)
    USB_DRV usb;
#endif //STM32_USB_DRIVER
#if (STM32_ETH_DRIVER)
    ETH_DRV eth;
#endif //STM32_ETH_DRIVER
#if (STM32_SDMMC_DRIVER)
    SDMMC_DRV sdmmc;
#endif //STM32_SDMMC_DRIVER
#if (STM32_RNG_DRIVER)
    RNG_DRV rng;
#endif //STM32_RNG_DRIVER

}EXO;

#endif // STM32_EXO_PRIVATE_H
