/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_core.h"
#include "stm32_core_private.h"
#include "stm32_timer.h"
#include "stm32_pin.h"
#include "stm32_power.h"
#include "../../userspace/object.h"
#include "stm32_rtc.h"
#include "stm32_wdt.h"
#include "stm32_uart.h"
#include "stm32_adc.h"
#include "stm32_dac.h"
#include "stm32_eep.h"
#if (MONOLITH_USB)
#ifdef STM32F10X_CL
#include "stm32_otg.h"
#else
#include "stm32_usb.h"
#endif //STM32F10X_CL
#endif

void stm32_core();

const REX __STM32_CORE = {
    //name
    "STM32 core driver",
    //size
    STM32_CORE_PROCESS_SIZE,
    //priority - driver priority
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    stm32_core
};

void stm32_core_loop(CORE* core)
{
    IPC ipc;
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_POWER:
            stm32_power_request(core, &ipc);
            break;
        case HAL_PIN:
            stm32_pin_request(core, &ipc);
            break;
        case HAL_TIMER:
            stm32_timer_request(core, &ipc);
            break;
#if (STM32_RTC_DRIVER)
        case HAL_RTC:
            stm32_rtc_request(&ipc);
            break;
#endif // STM32_RTC_DRIVER
#if (STM32_WDT_DRIVER)
        case HAL_WDT:
            stm32_wdt_request(&ipc);
            break;
#endif //STM32_WDT_DRIVER
#if (MONOLITH_UART)
        case HAL_UART:
            stm32_uart_request(core, &ipc);
            break;
#endif //MONOLITH_UART
#if (STM32_ADC_DRIVER)
        case HAL_ADC:
            stm32_adc_request(core, &ipc);
            break;
#endif //STM32_ADC_DRIVER
#if (STM32_DAC_DRIVER)
        case HAL_DAC:
            stm32_dac_request(core, &ipc);
            break;
#endif //STM32_DAC_DRIVER
#if (STM32_EEP_DRIVER)
        case HAL_EEPROM:
            stm32_eep_request(core, &ipc);
            break;
#endif //STM32_EEP_DRIVER
#if (MONOLITH_USB)
        case HAL_USB:
#ifdef STM32F10X_CL
            stm32_otg_request(core, &ipc);
#else
            stm32_usb_request(core, &ipc);
#endif //STM32F10X_CL
            break;
#endif //MONOLITH_USB
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}

void stm32_core()
{
    CORE core;
    object_set_self(SYS_OBJ_CORE);

#if (STM32_WDT_DRIVER)
    stm32_wdt_pre_init();
#endif //STM32_WDT_DRIVER
    stm32_power_init(&core);
    stm32_timer_init(&core);
    stm32_pin_init(&core);
#if (STM32_RTC_DRIVER)
    stm32_rtc_init();
#endif //STM32_RTC_DRIVER
#if (STM32_WDT_DRIVER)
    stm32_wdt_init();
#endif //STM32_WDT_DRIVER
#if (MONOLITH_UART)
    stm32_uart_init(&core);
#endif
#if (STM32_ADC_DRIVER)
    stm32_adc_init(&core);
#endif //STM32_ADC_DRIVER
#if (STM32_DAC_DRIVER)
    stm32_dac_init(&core);
#endif //STM32_DAC_DRIVER
#if (STM32_EEP_DRIVER)
    stm32_eep_init(&core);
#endif //STM32_EEP_DRIVER
#if (MONOLITH_USB)
#ifdef STM32F10X_CL
    stm32_otg_init(&core);
#else
    stm32_usb_init(&core);
#endif //STM32F10X_CL
#endif //MONOLITH_USB

    stm32_core_loop(&core);
}
