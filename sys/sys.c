/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sys_config.h"
#include "sys.h"
#include "sys_call.h"
#include "../userspace/svc.h"
#include "../userspace/core/core.h"
#include "../userspace/lib/stdio.h"

//temporaily struct, before root fs will be ready
typedef struct {
    HANDLE power;
    HANDLE gpio;
    HANDLE timer;
    HANDLE uart;
}SYS_OBJECT;

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
        //processing before send response
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc.cmd = IPC_PONG;
            ipc_post(&ipc);
            break;
        //TODO remove after FS will be ready
        case SYS_GET_POWER:
            ipc.param1 = sys_object.power;
            ipc_post(&ipc);
            break;
        case SYS_GET_GPIO:
            ipc.param1 = sys_object.gpio;
            ipc_post(&ipc);
            break;
        case SYS_GET_TIMER:
            ipc.param1 = sys_object.timer;
            ipc_post(&ipc);
            break;
        case SYS_GET_UART:
            ipc.param1 = sys_object.uart;
            ipc_post(&ipc);
            break;
        case SYS_SET_POWER:
            sys_object.power = ipc.process;
            break;
        case SYS_SET_GPIO:
            sys_object.gpio = ipc.process;
            break;
        case SYS_SET_TIMER:
            sys_object.timer = ipc.process;
            break;
        case SYS_SET_UART:
            sys_object.uart = ipc.process;
            break;
        case SYS_SET_STDOUT:
            __HEAP->stdout = (STDOUT)ipc.param1;
            __HEAP->stdout_param = (void*)ipc.param2;
#if (SYS_DEBUG)
            printf("RExOS system v. 0.0.2 started\n\r");
#endif
            break;
        }
    }
}
