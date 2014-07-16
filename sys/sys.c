/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sys_config.h"
#include "sys.h"
#include "sys_call.h"
#include "../userspace/lib/stdio.h"

//temporaily struct, before root fs will be ready
typedef struct {
    HANDLE power, gpio, timer, uart, rtc;
    HANDLE stdout_stream;
    HANDLE stdin_stream;
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
    SYS sys;
    sys.power = sys.gpio = sys.timer = sys.uart = sys.rtc = INVALID_HANDLE;
    sys.stdout_stream = INVALID_HANDLE;
    sys.stdin_stream = INVALID_HANDLE;
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
        case IPC_CALL_ERROR:
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
            case SYS_OBJECT_RTC:
                ipc.param1 = sys.rtc;
                break;
            case SYS_OBJECT_STDOUT_STREAM:
                ipc.param1 = sys.stdout_stream;
                break;
            case SYS_OBJECT_STDIN_STREAM:
                ipc.param1 = sys.stdin_stream;
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
            case SYS_OBJECT_RTC:
                sys.rtc = ipc.process;
                break;
            case SYS_OBJECT_STDOUT_STREAM:
                sys.stdout_stream = ipc.param2;
                break;
            case SYS_OBJECT_STDIN_STREAM:
                sys.stdin_stream = ipc.param2;
                break;
            default:
                ipc.cmd = IPC_INVALID_PARAM;
                break;

            }
            ipc_post(&ipc);
            break;
        case SYS_SET_STDIO:
            if (sys.stdout_stream != INVALID_HANDLE)
                __HEAP->stdout = stream_open(sys.stdout_stream);
#if (SYS_INFO)
            printf("RExOS system v. 0.0.2 started\n\r");
#endif
            ipc_post(&ipc);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
        }
    }
}
