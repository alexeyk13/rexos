/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "stm32_exo.h"
#include "stm32_exo_private.h"
#include "stm32_timer.h"
#include "stm32_pin.h"
#include "stm32_power.h"
#include "stm32_rtc.h"
#include "stm32_wdt.h"
#include "stm32_uart.h"
#include "stm32_can.h"
#include "stm32_adc.h"
#include "stm32_dac.h"
#include "stm32_eep.h"
#include "stm32_eth.h"
#include "stm32_i2c.h"
#ifdef STM32F10X_CL
#include "stm32_otg.h"
#else
#include "stm32_usb.h"
#endif //STM32F10X_CL
#include "../kernel.h"
#include "../kstdlib.h"
#include "../kerror.h"

void exodriver_delay_us(unsigned int us)
{
    unsigned int i;
    for (i = get_core_clock_internal() / 5000000 * us; i; --i)
        __NOP();
}

void exodriver_post(IPC* ipc)
{
    switch (HAL_GROUP(ipc->cmd))
    {
    case HAL_POWER:
        stm32_power_request(__KERNEL->exo, ipc);
        break;
    case HAL_PIN:
        stm32_pin_request(__KERNEL->exo, ipc);
        break;
    case HAL_TIMER:
        stm32_timer_request(__KERNEL->exo, ipc);
        break;
#if (STM32_RTC_DRIVER)
    case HAL_RTC:
        stm32_rtc_request(ipc);
        break;
#endif // STM32_RTC_DRIVER
#if (STM32_WDT_DRIVER)
        case HAL_WDT:
        stm32_wdt_request(ipc);
        break;
#endif //STM32_WDT_DRIVER
#if (STM32_UART_DRIVER)
    case HAL_UART:
        stm32_uart_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_UART_DRIVER
#if (STM32_CAN_DRIVER)
        case HAL_CAN:
        stm32_can_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_CAN_DRIVER
#if (STM32_SPI_DRIVER)
        case HAL_SPI:
        stm32_spi_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_SPI_DRIVER
#if (STM32_ADC_DRIVER)
        case HAL_ADC:
        stm32_adc_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_ADC_DRIVER
#if (STM32_DAC_DRIVER)
        case HAL_DAC:
        stm32_dac_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_DAC_DRIVER
#if (STM32_EEP_DRIVER)
        case HAL_EEPROM:
        stm32_eep_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_EEP_DRIVER
#if (STM32_FLASH_DRIVER)
    case HAL_FLASH:
        stm32_flash_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_FLASH_DRIVER
#if (STM32_I2C_DRIVER)
        case HAL_I2C:
        stm32_i2c_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_I2C_DRIVER
#if (STM32_USB_DRIVER)
    case HAL_USB:
#ifdef STM32F10X_CL
        stm32_otg_request(__KERNEL->exo, ipc);
#else
        stm32_usb_request(__KERNEL->exo, ipc);
#endif //STM32F10X_CL
        break;
#endif //STM32_USB_DRIVER
#if (STM32_ETH_DRIVER)
        case HAL_ETH:
        stm32_eth_request(__KERNEL->exo, ipc);
        break;
#endif //STM32_ETH_DRIVER
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void exodriver_init()
{
//ISR disabled at this point
    __KERNEL->exo = kmalloc(sizeof(EXO));

#if (STM32_WDT_DRIVER)
    stm32_wdt_pre_init();
#endif //STM32_WDT_DRIVER
    stm32_power_init(__KERNEL->exo);
    stm32_timer_init(__KERNEL->exo);
    stm32_pin_init(__KERNEL->exo);
#if (STM32_RTC_DRIVER)
    stm32_rtc_init();
#endif //STM32_RTC_DRIVER
#if (STM32_WDT_DRIVER)
    stm32_wdt_init();
#endif //STM32_WDT_DRIVER
#if (STM32_UART_DRIVER)
    stm32_uart_init(__KERNEL->exo);
#endif //STM32_UART_DRIVER
#if (STM32_CAN_DRIVER)
    stm32_can_init(__KERNEL->exo);
#endif //STM32_CAN_DRIVER
#if (STM32_SPI_DRIVER)
    stm32_spi_init(__KERNEL->exo);
#endif //STM32_SPI_DRIVER
#if (STM32_ADC_DRIVER)
    stm32_adc_init(__KERNEL->exo);
#endif //STM32_ADC_DRIVER
#if (STM32_DAC_DRIVER)
    stm32_dac_init(__KERNEL->exo);
#endif //STM32_DAC_DRIVER
#if (STM32_EEP_DRIVER)
    stm32_eep_init(__KERNEL->exo);
#endif //STM32_EEP_DRIVER
#if (STM32_FLASH_DRIVER)
    stm32_flash_init(__KERNEL->exo);
#endif //STM32_FLASH_DRIVER
#if (STM32_I2C_DRIVER)
    stm32_i2c_init(__KERNEL->exo);
#endif //STM32_I2C_DRIVER
#if (STM32_USB_DRIVER)
#ifdef STM32F10X_CL
    stm32_otg_init(__KERNEL->exo);
#else
    stm32_usb_init(__KERNEL->exo);
#endif //STM32F10X_CL
#endif //STM32_USB_DRIVER
#if (STM32_ETH_DRIVER)
    stm32_eth_init(__KERNEL->exo);
#endif //STM32_ETH_DRIVER

}
