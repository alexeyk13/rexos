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
    STM32_CORE_STACK_SIZE,
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
    bool need_post;
    int group;
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        group = -1;
        ipc_read_ms(&ipc, 0, 0);
        if (ipc.cmd < IPC_SYSTEM)
        {
            switch (ipc.cmd)
            {
            case IPC_PING:
                need_post = true;
                break;
            case IPC_CALL_ERROR:
                break;
            case IPC_SET_STDIO:
                open_stdout();
                need_post = true;
                break;
#if (SYS_INFO)
            case IPC_GET_INFO:
                need_post |= stm32_gpio_request(core, &ipc);
                need_post |= stm32_timer_request(core, &ipc);
                need_post |= stm32_power_request(core, &ipc);
#if !(TIMER_SOFT_RTC)
                need_post |= stm32_rtc_request(&ipc);
#endif
                break;
#endif
            default:
                ipc_set_error(&ipc, ERROR_NOT_SUPPORTED);
                need_post = true;
            }
        }
        //prepare for UART, USB, Analog
        else if (ipc.cmd < IPC_USER)
        {
            ipc_set_error(&ipc, ERROR_NOT_SUPPORTED);
            need_post = true;
        }
        else
            group = HAL_IPC_GROUP(ipc.cmd);
        if (group >= 0)
        {
            switch (group)
            {
            case HAL_POWER:
                need_post = stm32_power_request(core, &ipc);
                break;
            case HAL_GPIO:
                need_post = stm32_gpio_request(core, &ipc);
                break;
            case HAL_TIMER:
                need_post = stm32_timer_request(core, &ipc);
                break;
#if !(TIMER_SOFT_RTC)
            case HAL_RTC:
                need_post = stm32_rtc_request(&ipc);
                break;
#endif //!TIMER_SOFT_RTC
#if (STM32_WDT)
            case HAL_WDT:
                need_post = stm32_wdt_request(&ipc);
                break;
#endif //STM32_WDT
            default:
                ipc_set_error(&ipc, ERROR_NOT_SUPPORTED);
                need_post = true;
                break;
            }
        }
        if (need_post)
            ipc_post_or_error(&ipc);
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
