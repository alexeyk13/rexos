/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "rndisd.h"
#include "usbd.h"
#include "../../userspace/sys.h"
#include "../../userspace/stdio.h"
#include "../../userspace/io.h"
#include "../../userspace/uart.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/cdc_acm.h"
#include "../../userspace/usb.h"
#include "sys_config.h"

#define CDC_BLOCK_SIZE                                                          64
#define CDC_NOTIFY_SIZE                                                         64

typedef struct {
    IO* rx;
    IO* tx;
    uint8_t data_ep;
    uint16_t data_ep_size, rx_free, tx_size;
    uint8_t tx_idle, data_iface, control_iface;
    bool suspended;
} RNDISD;

void rndisd_destroy(RNDISD* rndisd)
{
    io_destroy(rndisd->rx);
    io_destroy(rndisd->tx);

    free(rndisd);
}

void rndisd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_INTERFACE_DESCRIPTOR_TYPE* diface;
    CDC_UNION_DESCRIPTOR_TYPE* u;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    uint8_t data_ep, data_iface, control_iface;
    uint16_t data_ep_size;

    //check control/data ep here
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        //RNDIS only
        if ((iface->bInterfaceClass != CDC_COMM_INTERFACE_CLASS) || (iface->bInterfaceSubClass != CDC_ACM) || (iface->bInterfaceProtocol != CDC_CP_VENDOR))
            continue;
        control_iface = iface->bInterfaceNumber;
#if (USBD_RNDIS_DEBUG)
        printf("Found RNDIS device interface: %d\n", control_iface);
#endif //USBD_RNDIS_DEBUG
        //find union descriptor
        for (u = usb_interface_get_first_descriptor(cfg, iface, CS_INTERFACE); u != NULL; u = usb_interface_get_next_descriptor(cfg, u, CS_INTERFACE))
        {
            if ((u->bDescriptorSybType == CDC_DESCRIPTOR_UNION) && (u->bControlInterface == iface->bInterfaceNumber) &&
                (u->bFunctionLength > sizeof(CDC_UNION_DESCRIPTOR_TYPE)))
                break;
        }
        if (u == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning - no UNION descriptor, skipping interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
        data_iface = ((uint8_t*)u)[sizeof(CDC_UNION_DESCRIPTOR_TYPE)];
        diface = usb_find_interface(cfg, data_iface);
        if (diface == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning no data interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
#if (USBD_RNDIS_DEBUG)
        printf("Found RNDIS device data interface: %d\n", data_iface);
#endif //USBD_RNDIS_DEBUG

        ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, diface, USB_ENDPOINT_DESCRIPTOR_INDEX);
        if (ep == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning no data EP, skipping interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
        data_ep = USB_EP_NUM(ep->bEndpointAddress);
        data_ep_size = ep->wMaxPacketSize;

        //configuration is ok, applying
        RNDISD* rndisd = (RNDISD*)malloc(sizeof(RNDISD));
        if (rndisd == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Out of memory\n");
#endif //USBD_RNDIS_DEBUG
            return;
        }

        rndisd->control_iface = control_iface;
        rndisd->data_iface = data_iface;
        rndisd->data_ep = data_ep;
        rndisd->data_ep_size = data_ep_size;
        rndisd->tx = rndisd->rx = NULL;
        rndisd->suspended = false;

        usbd_usb_ep_open(usbd, USB_EP_IN | rndisd->data_ep, USB_EP_BULK, rndisd->data_ep_size);
        usbd_usb_ep_open(usbd, rndisd->data_ep, USB_EP_BULK, rndisd->data_ep_size);

        usbd_register_interface(usbd, rndisd->control_iface, &__RNDISD_CLASS, rndisd);
        usbd_register_interface(usbd, rndisd->data_iface, &__RNDISD_CLASS, rndisd);
        usbd_register_endpoint(usbd, rndisd->data_iface, rndisd->data_ep);
    }
}

void rndisd_class_reset(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;

    usbd_usb_ep_close(usbd, USB_EP_IN | rndisd->data_ep);
    usbd_usb_ep_close(usbd, rndisd->data_ep);

    usbd_unregister_endpoint(usbd, rndisd->data_iface, rndisd->data_ep);
    usbd_unregister_interface(usbd, rndisd->data_iface, &__RNDISD_CLASS);
    usbd_unregister_interface(usbd, rndisd->control_iface, &__RNDISD_CLASS);
    rndisd_destroy(rndisd);
}

void rndisd_class_suspend(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;
    usbd_usb_ep_flush(usbd, USB_EP_IN | rndisd->data_ep);
    usbd_usb_ep_flush(usbd, rndisd->data_ep);

    rndisd->suspended = true;
}

void rndisd_class_resume(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;
    rndisd->suspended = false;
}

static inline void rndisd_read_complete(USBD* usbd, RNDISD* rndisd)
{
    if (rndisd->suspended)
        return;

}

void rndisd_write(USBD* usbd, RNDISD* rndisd)
{
    if (!rndisd->tx_idle || rndisd->suspended)
        return;

}


int rndisd_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io)
{
    RNDISD* rndisd = (RNDISD*)param;
    int res = -1;
    switch (setup->bRequest)
    {
    default:
        //TODO: RNDIS here
        dump(setup, 8);
        dump(io_data(io), 0x18);
        break;
    }
    return res;
}

static inline void rndisd_driver_event(USBD* usbd, RNDISD* rndisd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        rndisd_read_complete(usbd, rndisd);
        break;
    case IPC_WRITE:
        rndisd_write(usbd, rndisd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void rndisd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    RNDISD* rndisd = (RNDISD*)param;
    if (HAL_GROUP(ipc->cmd) == HAL_USB)
        rndisd_driver_event(usbd, rndisd, ipc);
    else
        switch (HAL_ITEM(ipc->cmd))
        {
        default:
            error(ERROR_NOT_SUPPORTED);
        }
}

const USBD_CLASS __RNDISD_CLASS = {
    rndisd_class_configured,
    rndisd_class_reset,
    rndisd_class_suspend,
    rndisd_class_resume,
    rndisd_class_setup,
    rndisd_class_request,
};
