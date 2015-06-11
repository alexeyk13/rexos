/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "mscd.h"
#include "../../userspace/sys.h"
#include "../../userspace/stdio.h"
#include "../../userspace/io.h"
#include "../../userspace/uart.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/msc.h"
#include <stdint.h>
#include "usb.h"
#include "sys_config.h"

typedef enum  {
    MSCD_STATE_CBW = 0,
    MSCD_STATE_DATA_IN,
    MSCD_STATE_STORAGE,
    MSCD_STATE_DATA_OUT,
    MSCD_STATE_CSW
} MSCD_STATE;

typedef struct {
    uint8_t ep_num, iface_num;
    uint16_t ep_size;
    IO* control;
    IO* data;

    //in feature this will make LUN
    MSCD_STATE state;
    IO* in;
    IO* out;
    uint32_t tag;
} MSCD;

static void mscd_destroy(MSCD* mscd)
{
    io_destroy(mscd->control);
    io_destroy(mscd->data);
    free(mscd);
}

static void mscd_fatal(USBD* usbd, MSCD* mscd)
{
    usbd_usb_ep_set_stall(usbd, mscd->ep_num);
    usbd_usb_ep_set_stall(usbd, USB_EP_IN | mscd->ep_num);
}

void mscd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    unsigned int ep_num, ep_size, iface_num;
    ep_num = 0;

    //check control/data ep here
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX);
        if (ep != NULL && iface->bInterfaceClass == MSC_INTERFACE_CLASS)
        {
            ep_num = USB_EP_NUM(ep->bEndpointAddress);
            ep_size = ep->wMaxPacketSize;
            iface_num = iface->bInterfaceNumber;
            break;
        }
    }

    //No MSC interface
    if (ep_num == 0)
        return;
    MSCD* mscd = (MSCD*)malloc(sizeof(MSCD));
    if (mscd == NULL)
        return;

    mscd->iface_num = iface_num;
    mscd->ep_num = ep_num;
    mscd->ep_size = ep_size;

    mscd->in = mscd->out = NULL;
    mscd->state = MSCD_STATE_CBW;
    mscd->control = io_create(ep_size);
    //TODO: scsi stack size
    mscd->data = io_create(USBD_MSC_MAX_IO_SIZE);
    if (mscd->control == NULL || mscd->data == NULL)
    {
        mscd_destroy(mscd);
        return;
    }

#if (USBD_MSC_DEBUG_REQUESTS)
    printf("Found USB MSCD class, EP%d, size: %d, iface: %d\n\r", ep_num, ep_size, iface_num);
#endif //USBD_CDC_DEBUG_REQUESTS

    usbd_register_interface(usbd, iface_num, &__MSCD_CLASS, mscd);
    usbd_register_endpoint(usbd, iface_num, ep_num);

    usbd_usb_ep_open(usbd, USB_EP_IN | ep_num, USB_EP_BULK, ep_size);
    usbd_usb_ep_open(usbd, ep_num, USB_EP_BULK, ep_size);

    usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
}

void mscd_class_reset(USBD* usbd, void* param)
{
    MSCD* mscd = (MSCD*)param;

    //TODO:

    mscd_destroy(mscd);
}

void mscd_class_suspend(USBD* usbd, void* param)
{
    MSCD* mscd = (MSCD*)param;
    //TODO:
}

void mscd_class_resume(USBD* usbd, void* param)
{
    MSCD* mscd = (MSCD*)param;
    //TODO:
}

static inline int mscd_get_max_lun(USBD* usbd, MSCD* mscd, IO* io)
{
    *((uint8_t*)io_data(io)) = USBD_MSC_LUN_COUNT - 1;
    io->data_size = sizeof(uint8_t);
#if (USBD_MSC_DEBUG_REQUESTS)
    printf("MSCD: Get Max Lun - %d\n\r", USBD_MSC_LUN_COUNT - 1);
#endif //USBD_CDC_DEBUG_REQUESTS
    return sizeof(uint8_t);
}

static inline int mscd_mass_storage_reset(USBD* usbd, MSCD* mscd)
{
    //TODO:
#if (USBD_MSC_DEBUG_REQUESTS)
    printf("MSCD: Mass Storage Reset\n\r");
#endif //USBD_CDC_DEBUG_REQUESTS
    return 0;
}

int mscd_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io)
{
    MSCD* mscd = (MSCD*)param;
    int res = -1;
    switch (setup->bRequest)
    {
    case MSC_BO_GET_MAX_LUN:
        res = mscd_get_max_lun(usbd, mscd, io);
        break;
    case MSC_BO_MASS_STORAGE_RESET:
        res = mscd_mass_storage_reset(usbd, mscd);
        break;
    }
    return res;
}

static void mscd_scsi_request(USBD* usbd, MSCD* mscd)
{
    printd("time for scsi");

}

static inline void mscd_cbw(USBD* usbd, MSCD* mscd)
{
    CBW* cbw = io_data(mscd->control);
    if (mscd->control->data_size < sizeof(CBW) || cbw->dCBWSignature != MSC_CBW_SIGNATURE)
    {
#if (USBD_MSC_DEBUG_REQUESTS)
        printf("MSCD: Fatal - invalid CBW\n\r");
#endif //USBD_CDC_DEBUG_REQUESTS
        mscd_fatal(usbd, mscd);
    }
    mscd->tag = cbw->dCBWTag;
#if (USBD_MSC_DEBUG_IO)
    printf("USB_MSC: dCBWTag: %d\n\r", cbw->dCBWTag);
    printf("USB_MSC: dCBWDataTransferLength: %d\n\r", cbw->dCBWDataTransferLength);
    printf("USB_MSC: dCBWDataFlags: %02x\n\r", cbw->bmCBWFlags);
    printf("USB_MSC: dCBWLUN: %d\n\r", cbw->bCBWLUN);
    //remove me
    printf("USB_MSC: dCBWCB:");
    int i;
    for (i = 0; i < cbw->bCBWCBLength; ++i)
        printf(" %02x", cbw->CBWCB[i]);
    printf(" (%d)\n\r", cbw->bCBWCBLength);
#endif //USBD_MSC_DEBUG_IO
    if (MSC_CBW_FLAG_DATA_IN(cbw->bmCBWFlags) || cbw->dCBWDataTransferLength == 0)
        mscd_scsi_request(usbd, mscd);
    else
    {
        mscd->state = MSCD_STATE_DATA_OUT;
        usbd_usb_ep_read(usbd, mscd->ep_num, mscd->data, cbw->dCBWDataTransferLength);
    }
}

static inline void mscd_usb_rx(USBD* usbd, MSCD* mscd)
{
    switch (mscd->state)
    {
    case MSCD_STATE_CBW:
        mscd_cbw(usbd, mscd);
        break;
    case MSCD_STATE_DATA_OUT:
        mscd_scsi_request(usbd, mscd);
        break;
    default:
#if (USBD_MSC_DEBUG_REQUESTS)
        printf("MSCD: Fatal - invalid stage - %d\n\r", mscd->state);
#endif //USBD_CDC_DEBUG_REQUESTS
        mscd_fatal(usbd, mscd);
    }
}

static inline bool mscd_driver_event(USBD* usbd, MSCD* mscd, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        mscd_usb_rx(usbd, mscd);
        break;
    case IPC_WRITE:
        printd("todo\n\r");
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}

bool mscd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    MSCD* mscd = (MSCD*)param;
    bool need_post = false;
    if (HAL_GROUP(ipc->cmd) == HAL_USB)
        need_post = mscd_driver_event(usbd, mscd, ipc);
    else
        switch (HAL_ITEM(ipc->cmd))
        {
        //TODO: interface request
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
        }
    return need_post;
}

const USBD_CLASS __MSCD_CLASS = {
    mscd_class_configured,
    mscd_class_reset,
    mscd_class_suspend,
    mscd_class_resume,
    mscd_class_setup,
    mscd_class_request,
};
