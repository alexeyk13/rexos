/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef NRF_EXO_PRIVATE_H_
#define NRF_EXO_PRIVATE_H_


#include "nrf_config.h"
#include "nrf_exo.h"
#include "nrf_pin.h"
#include "nrf_timer.h"
#include "nrf_power.h"
#if (NRF_UART_DRIVER)
#include "nrf_uart.h"
#endif // NRF_UART_DRIVER
#if (NRF_ADC_DRIVER)
#include "nrf_adc.h"
#endif // NRF_ADC_DRIVER
#if (NRF_SPI_DRIVER)
#include "nrf_spi.h"
#endif //NRF_SPI_DRIVER
#if (NRF_WDT_DRIVER)
#include "nrf_wdt.h"
#endif // NRF_WDT_DRIVER
#if (NRF_FLASH_DRIVER)
#include "nrf_flash.h"
#endif // NRF_FLASH_DRIVER
#if (NRF_RTC_DRIVER)
#include "nrf_rtc.h"
#endif // NRF_UART_DRIVER
#if (NRF_RF_DRIVER)
#include "nrf_rf.h"
#endif // NRF_RF_DRIVER
#if (NRF_BLE_CONTROLLER_DRIVER)
#include "nrf_ble_contr_private.h"
#endif // NRF_BLE_CONTROLLER_DRIVER
#if (NRF_RNG_DRIVER)
#include "nrf_rng.h"
#endif // NRF_RNG_DRIVER
#if (NRF_TEMP_DRIVER)
#include "nrf_temp.h"
#endif // NRF_TEMP_DRIVER

typedef struct _EXO {
    GPIO_DRV gpio;
#if (NRF_TIMER_DRIVER)
    TIMER_DRV timer;
#endif // NRF_TIMER_DRIVER
    POWER_DRV power;
#if (NRF_UART_DRIVER)
    UART_DRV uart;
#endif //NRF_UART_DRIVER
#if (NRF_SPI_DRIVER)
    SPI_DRV spi;
#endif //NRF_SPI_DRIVER
#if (NRF_ADC_DRIVER)
    ADC_DRV adc;
#endif //NRF_ADC_DRIVER
#if (NRF_FLASH_DRIVER)
    FLASH_DRV flash;
#endif //NRF_FLASH_DRIVER
#if (NRF_RADIO_DRIVER)
    RADIO_DRV radio;
#endif // NRF_RADIO_DRIVER
#if (NRF_RTC_DRIVER)
    RTC_DRV rtc;
#endif // NRF_RTC_DRIVER
#if (NRF_RF_DRIVER)
    RADIO_DRV rf;
#endif // NRF_RF_DRIVER
#if (NRF_BLE_CONTROLLER_DRIVER)
    BLE_CONTR_DRV rf;
#endif // NRF_BLE_CONTROLLER_DRIVER
#if (NRF_RNG_DRIVER)
    RNG_DRV rng;
#endif // nrf_rng_irq
}EXO;


#endif /* NRF_EXO_PRIVATE_H_ */
