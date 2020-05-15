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
#if (NRF_BLE_CONTROLLER_DRIVER)
    case HAL_BLE_CONTR:
        nrf_ble_request(__KERNEL->exo, ipc);
        break;
#endif // NRF_BLE_CONTROLLER_DRIVER
    case HAL_POWER:
        nrf_power_request(__KERNEL->exo, ipc);
        break;
    case HAL_PIN:
        nrf_pin_request(__KERNEL->exo, ipc);
        break;
#if (NRF_TIMER_DRIVER)
    case HAL_TIMER:
        nrf_timer_request(__KERNEL->exo, ipc);
        break;
#endif // NRF_TIMER_DRIVER
#if (NRF_RTC_DRIVER)
    case HAL_RTC:
        nrf_rtc_request(__KERNEL->exo, ipc);
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
#if (NRF_RF_DRIVER)
    case HAL_RF:
        nrf_rf_request(__KERNEL->exo, ipc);
        break;
#endif // NRF_RF_DRIVER
#if (NRF_RNG_DRIVER)
    case HAL_RNG:
        nrf_rng_request(__KERNEL->exo, ipc);
        break;
#endif // NRF_RNG_DRIVER
#if (NRF_TEMP_DRIVER)
    case HAL_TEMP:
        nrf_temp_request(__KERNEL->exo, ipc);
        break;
#endif // NRF_TEMP_DRIVER
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void exodriver_init()
{
    //ISR disabled at this point
    __KERNEL->exo = kmalloc(sizeof(EXO));

#if (NRF_SRAM_POWER_CONFIG)
#if (NRF_RAM1_ENABLE)
    kstdlib_add_pool(SRAM_BASE + SRAM_SIZE, SRAM_SIZE
#if (NRF_RAM2_ENABLE)
            + SRAM_SIZE
#endif // NRF_RAM2_ENABLE
#if (NRF_RAM3_ENABLE)
            + SRAM_SIZE
#endif // NRF_RAM3_ENABLE
#if (NRF_RAM4_ENABLE)
            + SRAM_SIZE
#endif // NRF_RAM3_ENABLE
#if (NRF_RAM5_ENABLE)
            + SRAM_SIZE
#endif // NRF_RAM3_ENABLE
#if (NRF_RAM6_ENABLE)
            + SRAM_SIZE
#endif // NRF_RAM3_ENABLE
#if (NRF_RAM7_ENABLE)
            + SRAM_SIZE
#endif // NRF_RAM3_ENABLE
            );
#endif // NRF_RAM1_ENABLE
#endif // NRF_SRAM_POWER_CONFIG

#if (NRF_WDT_DRIVER)
    nrf_wdt_pre_init();
#endif // NRF_WDT_DRIVER
    nrf_power_init(__KERNEL->exo);
    nrf_pin_init(__KERNEL->exo);
    nrf_timer_init(__KERNEL->exo);
#if (NRF_RTC_DRIVER)
    nrf_rtc_init(__KERNEL->exo);
#endif // NRF_RTC_DRIVER
#if (NRF_ADC_DRIVER)
    nrf_adc_init(__KERNEL->exo);
#endif // NRF_ADC_DRIVER
#if (NRF_UART_DRIVER)
    nrf_uart_init(__KERNEL->exo);
#endif //NRF_UART_DRIVER
#if (NRF_FLASH_DRIVER)
    nrf_flash_init(__KERNEL->exo);
#endif //LPC_FLASH_DRIVER
#if (NRF_RF_DRIVER)
    nrf_rf_init(__KERNEL->exo);
#endif // NRF_RF_DRIVER
#if (NRF_BLE_CONTROLLER_DRIVER)
    nrf_ble_init(__KERNEL->exo);
#endif // NRF_BLE_CONTROLLER_DRIVER
#if (NRF_RNG_DRIVER)
    nrf_rng_init(__KERNEL->exo);
#endif // NRF_RNG_DRIVER
}
