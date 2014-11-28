/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_CORE_PRIVATE_H
#define LPC_CORE_PRIVATE_H

#include "lpc_config.h"
#include "lpc_core.h"
//#include "lpc_timer.h"
//#include "lpc_power.h"
//#if (MONOLITH_UART)
//#include "lpc_uart.h"
//#endif

typedef struct _CORE {
//    TIMER_DRV timer;
//    POWER_DRV power;
//#if (MONOLITH_UART)
//    UART_DRV uart;
//#endif
}CORE;

/*#if (SYS_INFO)
#if (UART_STDIO) && (MONOLITH_UART)
#define printd          printu
#else
#define printd          printf
#endif
#endif //SYS_INFO
*/

#endif // LPC_CORE_PRIVATE_H
