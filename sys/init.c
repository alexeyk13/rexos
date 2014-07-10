/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

/*
    init.c - default init file for RExOS SYS. Kernel can be compiled independently, thus totally rewritting following file
 */

#include "sys.h"
#include "sys_call.h"
#include "sys_config.h"
#include "../userspace/process.h"
#include "../userspace/lib/stdio.h"
#include "../userspace/ipc.h"

#if defined (STM32)
#include "drv/stm32_power.h"
#include "drv/stm32_gpio.h"
#include "drv/stm32_uart.h"
#include "drv/stm32_timer.h"
#endif
#include "sys_config.h"

extern const REX __SYS;

#if (SYS_APP)
extern const REX __APP;
#endif

void init(void);

const REX __INIT = {
    //name
    "INIT",
    //size
    512,
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
    __HEAP->system = process_create(&__SYS);

#if defined(STM32)
    process_create(&__STM32_POWER);
    process_create(&__STM32_GPIO);
    //todo: make process
    timer_init_hw();
#if (UART_MODULE)
    process_create(&__STM32_UART);
#endif

#else
#warning No drivers loaded. System is abstract!
#endif

    //start user application, if required
#if (SYS_APP)
    process_create(&__APP);
#endif

    for (;;)
    {
#if (SYS_POWERSAVE)
#if defined(CORTEX_M)
        __WFI();
#endif //CORTEX_M
#endif
    }
}
