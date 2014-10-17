/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sys.h"
#include "sys_config.h"
#include "../userspace/process.h"
#include "../userspace/lib/stdio.h"
#include "../userspace/ipc.h"

#include "sys_config.h"
#if defined (STM32)
#include "stm32_config.h"
#include "drv/stm32_core.h"
#if (UART_STDIO)
#include "drv/stm32_uart.h"
#endif //UART_STDIO
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
#if defined(STM32)
    process_create(&__STM32_CORE);
#if (UART_STDIO)
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
