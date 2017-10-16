/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_wdt.h"
#include "../../userspace/sys.h"
#include "../../userspace/wdt.h"
#include "../kerror.h"
#include "stm32_config.h"

#define KICK_KEY                                        0xaaaa
#define WRITE_ENABLE_KEY                                0x5555
#define START_KEY                                       0xcccc

void stm32_wdt_pre_init()
{
#if (HARDWARE_WATCHDOG)
    //kick watchdog
    IWDG->KR = KICK_KEY;
#else
    //check/turn on LSI
    RCC->CSR |= RCC_CSR_LSION;
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0) {}
#endif
    //set prescaller to max for RTC set up
    __disable_irq();
    IWDG->KR = WRITE_ENABLE_KEY;
    IWDG->PR = 7;
    IWDG->KR = WRITE_ENABLE_KEY;
    IWDG->RLR = 0xfff;
    __enable_irq();

    //start if not started by hardware
#if !(HARDWARE_WATCHDOG)
    //start WDT
    IWDG->KR = START_KEY;
#endif
    //kick watchdog
    IWDG->KR = KICK_KEY;
}

void stm32_wdt_init()
{
    //kick watchdog
    IWDG->KR = KICK_KEY;
    __disable_irq();
    IWDG->KR = WRITE_ENABLE_KEY;
    //1.2..2.4 sec
    IWDG->PR = 2;
    __enable_irq();
    //kick watchdog
    IWDG->KR = KICK_KEY;
}

void stm32_wdt_kick()
{
    IWDG->KR = KICK_KEY;
}

void stm32_wdt_request(IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case WDT_KICK:
        stm32_wdt_kick();
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

