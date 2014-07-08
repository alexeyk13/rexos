/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

/*
    init.c - default init file for RExOS SYS. Kernel can be compiled independently, thus totally rewritting following file
 */

#include "sys_config.h"
#include "../userspace/process.h"

#if defined (STM32)
#include "drv/stm32_power.h"
#include "drv/stm32_uart.h"
#include "drv/stm32_systimer.h"
#endif

extern const REX __SYS;

#if (SYS_APP)
extern const REX __APP;
#endif

void init(void);

const REX __INIT = {
    //name
    "INIT",
    //size
    256,
    //priority - init priority
    ((unsigned int)-1),
    //flags - init must be called frozen)
    0,
    //ipc size - no ipc
    0,
    //function
    init
};

void init()
{
    //start the system
    process_create(&__SYS);

#if defined(STM32)
    process_create(&__STM32_POWER);
    timer_init_hw();
    process_create(&__STM32_UART);
#else
#warning No drivers loaded. System is abstract!
#endif

    //start user application, if required
#if (SYS_APP)
    process_create(&__APP);
#endif

    for (;;)
    {
        //TODO WFI here
    }
}
