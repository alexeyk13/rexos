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
}SYS;

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
    SYS sys = {0};
    IPC ipc;
    setup_system();
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        //processing before send response
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        //Temporaily solution. Remove after FS will be ready
        case SYS_GET_OBJECT:
            switch (ipc.param1)
            {
            case SYS_OBJECT_POWER:
                ipc.param1 = sys.power;
                break;
            case SYS_OBJECT_GPIO:
                ipc.param1 = sys.gpio;
                break;
            case SYS_OBJECT_TIMER:
                ipc.param1 = sys.timer;
                break;
            case SYS_OBJECT_UART:
                ipc.param1 = sys.uart;
                break;
            default:
                ipc.param1 = (unsigned int)INVALID_HANDLE;
                ipc.cmd = IPC_INVALID_PARAM;
                break;

            }
            ipc_post(&ipc);
            break;
        case SYS_SET_OBJECT:
            switch (ipc.param1)
            {
            case SYS_OBJECT_POWER:
                sys.power = ipc.process;
                break;
            case SYS_OBJECT_GPIO:
                sys.gpio = ipc.process;
                break;
            case SYS_OBJECT_TIMER:
                sys.timer = ipc.process;
                break;
            case SYS_OBJECT_UART:
                sys.uart = ipc.process;
                break;
            default:
                ipc.cmd = IPC_INVALID_PARAM;
                break;

            }
            ipc_post(&ipc);
            break;
        case SYS_SET_STDOUT:
            __HEAP->stdout = (STDOUT)ipc.param1;
            __HEAP->stdout_param = (void*)ipc.param2;
#if (SYS_DEBUG)
            printf("RExOS system v. 0.0.2 started\n\r");
#endif
            ipc_post(&ipc);
            break;
        }
    }
}
