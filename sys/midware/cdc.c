/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "cdc.h"
#include "../sys.h"
#include "../file.h"
#include "../usb.h"
#include "../../userspace/lib/stdio.h"
#include "../../userspace/block.h"
#include "../../userspace/direct.h"
#include "sys_config.h"

#define CDC_BLOCK_SIZE                      64

typedef struct {
    SETUP setup;
    HANDLE usbd, usb;
    HANDLE block;
    unsigned int data_ep, control_ep;
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

static inline void cdc_open(CDC* cdc, HANDLE usbd, CDC_OPEN_STRUCT* cdc_open_struct)
{
    if (cdc->usbd != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    if ((cdc->block = block_create(CDC_BLOCK_SIZE)) == 0)
        return;
    cdc->usbd = usbd;
    ack(cdc->usbd, USBD_REGISTER_CLASS, 0, 0, 0);
    cdc->usb = get(cdc->usbd, USBD_GET_DRIVER, 0, 0, 0);
    cdc->data_ep = cdc_open_struct->data_ep;
    cdc->control_ep = cdc_open_struct->control_ep;
}

static inline void cdc_close(CDC* cdc)
{
    if (cdc->usbd == INVALID_HANDLE)
        return;

    fclose(cdc->usb, cdc->data_ep);
    fclose(cdc->usb, USB_EP_IN | cdc->data_ep);
    fclose(cdc->usb, USB_EP_IN | cdc->control_ep);

    ack(cdc->usbd, USBD_UNREGISTER_CLASS, 0, 0, 0);
    cdc->usbd = INVALID_HANDLE;
    cdc->usb = INVALID_HANDLE;
}

static inline void cdc_reset(CDC* cdc)
{
    fclose(cdc->usb, cdc->data_ep);
    fclose(cdc->usb, USB_EP_IN | cdc->data_ep);
    fclose(cdc->usb, USB_EP_IN | cdc->control_ep);
}

static inline void cdc_configured(CDC* cdc)
{
    USB_EP_OPEN ep_open;
    //data
    ep_open.size = 64;
    ep_open.type = USB_EP_BULK;
    fopen_ex(cdc->usb, cdc->data_ep, (void*)&ep_open, sizeof(USB_EP_OPEN));
    fopen_ex(cdc->usb, USB_EP_IN | cdc->data_ep, (void*)&ep_open, sizeof(USB_EP_OPEN));

    //control
    ep_open.size = 64;
    ep_open.type = USB_EP_INTERRUPT;
    fopen_ex(cdc->usb, USB_EP_IN | cdc->control_ep, (void*)&ep_open, sizeof(USB_EP_OPEN));

    fread(cdc->usb, cdc->data_ep, cdc->block, 64);
}

static inline bool cdc_setup(CDC* cdc)
{
    printf("CDC got SETUP, request code: %#X\n\r", cdc->setup.bRequest);
    return false;
}

void cdc()
{
    IPC ipc;
    CDC cdc;
    open_stdout();
    CDC_OPEN_STRUCT cdc_open_struct;

    cdc.usbd = cdc.usb = cdc.block = INVALID_HANDLE;
    cdc.data_ep = cdc.control_ep = INVALID_HANDLE;

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
            if (direct_read(ipc.process, (void*)&cdc_open_struct, sizeof(CDC_OPEN_STRUCT)))
                cdc_open(&cdc, (HANDLE)ipc.param1, &cdc_open_struct);
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
            cdc_close(&cdc);
            ipc_post_or_error(&ipc);
            break;
        case USB_RESET:
            cdc_reset(&cdc);
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
            cdc_configured(&cdc);
            //no response required
            break;
        case USB_SETUP:
            ((uint32_t*)(&cdc.setup))[0] = ipc.param1;
            ((uint32_t*)(&cdc.setup))[1] = ipc.param2;
            ipc.param1 = cdc_setup(&cdc);
            ipc_post_or_error(&ipc);
            break;
        case IPC_READ_COMPLETE:
            printf("CDC READ OK! \n\r");
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

