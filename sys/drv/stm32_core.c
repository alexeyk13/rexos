/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_core.h"
#include "stm32_core_private.h"
#include "stm32_timer.h"
#include "stm32_gpio.h"
#include "stm32_power.h"
#include "../../userspace/object.h"
#if !(TIMER_SOFT_RTC)
#include "stm32_rtc.h"
#endif
#if (STM32_WDT)
#include "stm32_wdt.h"
#endif

void stm32_core();

const REX __STM32_CORE = {
    //name
    "STM32 core driver",
    //size
    512,
    //priority - driver priority
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    stm32_core
};

void stm32_core_loop(CORE* core)
{
    IPC ipc;
    for (;;)
    {
        error(ERROR_OK);
        ipc_read_ms(&ipc, 0, 0);
        if (HAL_IPC_GROUP(ipc.cmd) == HAL_GPIO)
            stm32_gpio_request(core, &ipc);
        else
            switch (ipc.cmd)
            {
            case IPC_PING:
                ipc_post(&ipc);
                break;
            case IPC_CALL_ERROR:
                break;
            case IPC_SET_STDIO:
                open_stdout();
                ipc_post(&ipc);
                break;
    #if (SYS_INFO)
            case IPC_GET_INFO:
                stm32_gpio_info(core);
                stm32_timer_info();
                stm32_power_info(core);
    #if !(TIMER_SOFT_RTC)
                stm32_rtc_info();
    #endif
                ipc_post(&ipc);
                break;
    #endif
            //timer specific
            case STM32_TIMER_ENABLE:
                stm32_timer_enable(core, (TIMER_NUM)ipc.param1, ipc.param2);
                ipc_post_or_error(&ipc);
                break;
            case STM32_TIMER_DISABLE:
                stm32_timer_disable(core, (TIMER_NUM)ipc.param1);
                ipc_post_or_error(&ipc);
                break;
            case STM32_TIMER_ENABLE_EXT_CLOCK:
                stm32_timer_enable_ext_clock(core, (TIMER_NUM)ipc.param1, (PIN)ipc.param2, ipc.param3);
                ipc_post_or_error(&ipc);
                break;
            case STM32_TIMER_DISABLE_EXT_CLOCK:
                stm32_timer_disable_ext_clock(core, (TIMER_NUM)ipc.param1, (PIN)ipc.param2);
                ipc_post_or_error(&ipc);
                break;
            case STM32_TIMER_SETUP_HZ:
                stm32_timer_setup_hz((TIMER_NUM)ipc.param1, ipc.param2);
                ipc_post_or_error(&ipc);
                break;
            case STM32_TIMER_START:
                stm32_timer_start((TIMER_NUM)ipc.param1);
                ipc_post_or_error(&ipc);
                break;
            case STM32_TIMER_STOP:
                stm32_timer_stop(ipc.param1);
                ipc_post_or_error(&ipc);
                break;
            case STM32_TIMER_GET_CLOCK:
                ipc.param1 = stm32_timer_get_clock(ipc.param1);
                ipc_post_or_error(&ipc);
                break;
            //power specific
            case STM32_POWER_GET_CLOCK:
                ipc.param1 = get_clock(ipc.param1);
                ipc_post_or_error(&ipc);
                break;
            case STM32_POWER_UPDATE_CLOCK:
                update_clock(ipc.param1, ipc.param2, ipc.param3);
                ipc_post_or_error(&ipc);
                break;
            case STM32_POWER_GET_RESET_REASON:
                ipc.param1 = get_reset_reason(core);
                ipc_post_or_error(&ipc);
                break;
    #if defined(STM32F1)
            case STM32_POWER_DMA_ON:
                dma_on(core, ipc.param1);
                ipc_post_or_error(&ipc);
                break;
            case STM32_POWER_DMA_OFF:
                dma_off(core, ipc.param1);
                ipc_post_or_error(&ipc);
                break;
    #endif //STM32F1
    #if defined(STM32F1)
            case STM32_POWER_USB_ON:
                stm32_usb_power_on();
                ipc_post_or_error(&ipc);
                break;
            case STM32_POWER_USB_OFF:
                stm32_usb_power_off(core);
                ipc_post_or_error(&ipc);
                break;
    #endif //STM32F1
            //RTC
    #if !(TIMER_SOFT_RTC)
            case STM32_RTC_GET:
                ipc.param1 = (unsigned int)stm32_rtc_get();
                ipc_post(&ipc);
                break;
            case STM32_RTC_SET:
                stm32_rtc_set(core, ipc.param1);
                ipc_post(&ipc);
                break;
    #endif //!TIMER_SOFT_RTC
    #if (STM32_WDT)
            case STM32_WDT_KICK:
                stm32_wdt_kick();
                ipc_post(&ipc);
                break;
    #endif //STM32_WDT
            default:
                ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
                break;
            }
    }
}

void stm32_core()
{
    CORE core;
    object_set_self(SYS_OBJ_CORE);

#if (STM32_WDT)
    stm32_wdt_pre_init();
#endif
    stm32_power_init(&core);
    stm32_timer_init(&core);
    stm32_gpio_init(&core);
#if !(TIMER_SOFT_RTC)
    stm32_rtc_init();
#endif //!TIMER_SOFT_RTC
#if (STM32_WDT)
    stm32_wdt_init();
#endif

    stm32_core_loop(&core);
}
