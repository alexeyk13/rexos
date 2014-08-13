/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "cdc.h"
#include "../sys.h"
#include "../usb.h"
#include "../../userspace/lib/stdio.h"

#include "../../userspace/block.h"

typedef struct {
    HANDLE usbd, usb;
} CDC;

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

static inline void cdc_open(CDC* cdc, HANDLE usbd)
{
    if (cdc->usbd != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    cdc->usbd = usbd;
    ack(cdc->usbd, USBD_REGISTER_CLASS, 0, 0, 0);
    cdc->usb = get(cdc->usbd, USBD_GET_DRIVER, 0, 0, 0);
}

void cdc()
{
    IPC ipc;
    CDC cdc;
    open_stdout();

    cdc.usbd = cdc.usb = INVALID_HANDLE;

    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        error(ERROR_OK);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
//            cdc_info();
            ipc_post(&ipc);
            break;
#endif
        case IPC_OPEN:
            cdc_open(&cdc, (HANDLE)ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_RESET:
            printf("CDC: got USB reset\n\r");
            //no response required
            break;
        case USB_SUSPEND:
            printf("CDC: got USB suspend\n\r");
            //no response required
            break;
        case USB_WAKEUP:
            printf("CDC: got USB wakeup\n\r");
            //no response required
            break;
        case USB_CONFIGURED:
            printf("CDC: configured\n\r");
            //no response required
            break;
        case USB_SETUP:
            printf("CDC: SETUP received\n\r");
            //TODO: respond with state
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

