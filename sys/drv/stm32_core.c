/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_core.h"
#include "stm32_timer.h"
#include "stm32_gpio.h"
#include "stm32_power.h"
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
    PROCESS_FLAGS_ACTIVE,
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
        //GPIO specific
        case STM32_GPIO_ENABLE_PIN:
            gpio_enable_pin(core, (PIN)ipc.param1, (PIN_MODE)ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        case STM32_GPIO_ENABLE_PIN_SYSTEM:
#if defined(STM32F1)
            gpio_enable_pin_system(core, (PIN)ipc.param1, (GPIO_MODE)ipc.param2, ipc.param3);
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
            gpio_enable_pin_system(core, (PIN)ipc.param1, ipc.param2, (AF)ipc.param3);
#endif
            ipc_post_or_error(&ipc);
            break;
        case STM32_GPIO_DISABLE_PIN:
            gpio_disable_pin(core, (PIN)ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case STM32_GPIO_ENABLE_EXTI:
            gpio_enable_exti(core, (PIN)ipc.param1, ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        case STM32_GPIO_DISABLE_EXTI:
            gpio_disable_exti(core, (PIN)ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case STM32_GPIO_SET_PIN:
            gpio_set_pin((PIN)ipc.param1, (bool)ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        case STM32_GPIO_GET_PIN:
            ipc.param1 = gpio_get_pin((PIN)ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case STM32_GPIO_DISABLE_JTAG:
            gpio_disable_jtag(core);
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
        case STM32_POWER_BACKUP_ON:
            backup_on(core);
            ipc_post_or_error(&ipc);
            break;
        case STM32_POWER_BACKUP_OFF:
            backup_off(core);
            ipc_post_or_error(&ipc);
            break;
        case STM32_POWER_BACKUP_WRITE_ENABLE:
            backup_write_enable(core);
            ipc_post_or_error(&ipc);
            break;
        case STM32_POWER_BACKUP_WRITE_PROTECT:
            backup_write_protect(core);
            ipc_post_or_error(&ipc);
            break;
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
    sys_ack(IPC_SET_OBJECT, SYS_OBJECT_CORE, 0, 0);

#if (STM32_WDT)
    stm32_wdt_pre_init();
#endif
    stm32_power_init(&core);
    stm32_timer_init(&core);
    stm32_gpio_init(&core);
#if !(TIMER_SOFT_RTC)
    stm32_rtc_init(&core);
#endif //!TIMER_SOFT_RTC
#if (STM32_WDT)
    stm32_wdt_init();
#endif

    stm32_core_loop(&core);
}
