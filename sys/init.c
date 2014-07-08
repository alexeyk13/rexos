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
    HANDLE power, uart;
    //start the system
    __HEAP->system = process_create(&__SYS);

#if defined(STM32)
    power = process_create(&__STM32_POWER);
    sys_post(SYS_SET_POWER, power, 0, 0);
    //TODO: gpio here
    timer_init_hw();
    uart = process_create(&__STM32_UART);
    sys_post(SYS_SET_UART, uart, 0, 0);

#else
#warning No drivers loaded. System is abstract!
#endif

    //say early processes, that STDOUT is setted up
    __HEAP->stdout = (STDOUT)uart_write_svc;
    __HEAP->stdout_param = (void*)1;
    sys_post(SYS_SET_STDOUT, (unsigned int)uart_write_svc, (unsigned int)1, 0);
    post(power, SYS_SET_STDOUT, (unsigned int)uart_write_svc, (unsigned int)1, 0);

    //start user application, if required
#if (SYS_APP)
    process_create(&__APP);
#endif

    for (;;)
    {
#if (SYS_POWERSAVE)
#if defined(CORTEX_M)
        __WFI();
        //TODO WFI here
#endif //CORTEX_M
#endif
    }
}
