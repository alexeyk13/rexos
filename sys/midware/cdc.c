/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "cdc.h"
#include "../sys.h"
#include "../../userspace/lib/stdio.h"

void cdc();

const REX __CDC = {
    //name
    "CDC USB class",
    //size
    512,
    //priority - midware priority
    150,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    cdc
};

void cdc()
{
    HANDLE usb;
    IPC ipc;
    open_stdout();
    usb = sys_get_object(SYS_OBJECT_USB);

    ack(usb, SYS_USB_REGISTER_CLASS, 0, 0, 0);
    ack(usb, SYS_USB_ENABLE, 0, 0, 0);
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case SYS_GET_INFO:
//            cdc_info();
            ipc_post(&ipc);
            break;
#endif
        case SYS_USB_RESET:
            printf("got USB reset\n\r");
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

