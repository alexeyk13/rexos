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
    char* ptr;
    int i;

    HANDLE usb;
    HANDLE block;
    IPC ipc;
    open_stdout();
    usb = 0;//sys_get_object(SYS_OBJECT_USB);

    block = block_create(256);

    ack(usb, USB_REGISTER_DEVICE, 0, 0, 0);
    ack(usb, USB_ENABLE, 0, 0, 0);
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
        case USB_RESET:
            printf("got USB reset\n\r");
            block_send(block, usb);
            //we only waiting for setup packet now
            ack(usb, USB_EP_READ, block, 0, 8);
            break;
        case USB_SUSPEND:
            printf("got USB suspend\n\r");
            break;
        case USB_WAKEUP:
            printf("got USB wakeup\n\r");
            break;
        case USB_EP_READ_COMPLETE:
            printf("read complete\n\r");
            block_send(block, usb);
            //we only waiting for setup packet now
            ack(usb, USB_EP_READ, block, 0, 64);
            break;
        case USB_SETUP:
            printf("SETUP received\n\r");
            ptr = block_open(block);
            for (i = 0; i < 8; ++i)
            {
                printf("%#X ", ptr[i]);
            }
            printf("\n\r");
            block_send(block, usb);
            //we only waiting for setup packet now
            ack(usb, USB_EP_READ, block, 0, 8);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

