/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/


#include "nrf_exo.h"
#include "nrf_exo_private.h"
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
        nrf_power_request(__KERNEL->exo, ipc);
        break;
    case HAL_PIN:
        nrf_pin_request(__KERNEL->exo, ipc);
        break;
    case HAL_TIMER:
//        nrf_timer_request(__KERNEL->exo, ipc);
        break;
#if (NRF_RTC_DRIVER)
    case HAL_RTC:
        //nrf_rtc_request(ipc);
        break;
#endif // NRF_RTC_DRIVER
#if (NRF_WDT_DRIVER)
        case HAL_WDT:
        nrf_wdt_request(ipc);
        break;
#endif //NRF_WDT_DRIVER
#if (NRF_UART_DRIVER)
    case HAL_UART:
        nrf_uart_request(__KERNEL->exo, ipc);
        break;
#endif //NRF_UART_DRIVER
#if (NRF_SPI_DRIVER)
        case HAL_SPI:
        nrf_spi_request(__KERNEL->exo, ipc);
        break;
#endif //NRF_SPI_DRIVER
#if (NRF_ADC_DRIVER)
        case HAL_ADC:
        nrf_adc_request(__KERNEL->exo, ipc);
        break;
#endif //NRF_ADC_DRIVER
#if (NRF_FLASH_DRIVER)
    case HAL_FLASH:
        nrf_flash_request(__KERNEL->exo, ipc);
        break;
#endif //NRF_FLASH_DRIVER
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void exodriver_init()
{
    //ISR disabled at this point
    __KERNEL->exo = kmalloc(sizeof(EXO));

    nrf_power_init(__KERNEL->exo);
    nrf_pin_init(__KERNEL->exo);
    nrf_timer_init(__KERNEL->exo);

#if (NRF_UART_DRIVER)
    nrf_uart_init(__KERNEL->exo);
#endif //NRF_UART_DRIVER
#if (NRF_FLASH_DRIVER)
    nrf_flash_init(__KERNEL->exo);
#endif //LPC_FLASH_DRIVER
}
