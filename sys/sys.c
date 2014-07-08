/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sys_config.h"
#include "sys.h"
#include "../userspace/svc.h"
#include "../userspace/core/core.h"
#include "../userspace/lib/stdio.h"

#include "drv/stm32_uart.h"

void sys();

const REX __SYS = {
    //name
    "RExOS SYS",
    //size
    512,
    //priority - sys priority
    101,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    sys
};

void sys ()
{
    SYS_OBJECT sys_object = {0};
    IPC ipc;
    setup_system();

    for (;;)
    {
        ipc_wait_peek_ms(&ipc, 0, 0);
        if (ipc.cmd == IPC_UNKNOWN)
            continue;
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc.cmd = IPC_PONG;
            break;
        //TODO remove after FS will be ready
        case SYS_GET_POWER:
            ipc.param1 = sys_object.power;
            break;
        case SYS_GET_TIMER:
            ipc.param1 = sys_object.timer;
            break;
        case SYS_GET_UART:
            ipc.param1 = sys_object.uart;
            break;
        case SYS_SET_POWER:
            sys_object.power = ipc.process;
            break;
        case SYS_SET_TIMER:
            sys_object.timer = ipc.process;
        case SYS_SET_UART:
            sys_object.timer = ipc.process;
            __HEAP->stdout = uart_write_svc;
            __HEAP->stdout_param = (void*)1;
#if (SYS_DEBUG)
            printf("RExOS system v. 0.0.2 started\n\r");

#endif
            break;
        default:
            ipc.cmd = IPC_UNKNOWN;
        }
        ipc_post(&ipc);
    }
}
