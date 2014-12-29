/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "hidd.h"
#include "../userspace/hid.h"
#include "../userspace/stdlib.h"
#include "../userspace/file.h"
#include "../userspace/stdio.h"
#include "../userspace/block.h"
#include "../userspace/sys.h"

typedef struct {
    HANDLE usb;
    HANDLE block;
    unsigned int idle;
    uint16_t in_ep_size;
    uint8_t in_ep, iface;
    uint8_t suspended;
} HIDD;

static void hidd_destroy(HIDD* hidd)
{
    //TODO: free block
    free(hidd);
}

void hidd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    uint8_t in_ep, in_ep_size, hid_iface;
    unsigned int size;
    in_ep = in_ep_size = hid_iface = 0;

    //check control/data ep here
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        if (iface->bInterfaceClass == HID_INTERFACE_CLASS)
        {
            ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX);
            if (ep != NULL)
            {
                in_ep = USB_EP_NUM(ep->bEndpointAddress);
                in_ep_size = ep->wMaxPacketSize;
                hid_iface = iface->bInterfaceNumber;
                break;
            }
        }
    }

    //No HID descriptors in interface
    if (in_ep == 0)
        return;
    HIDD* hidd = (HIDD*)malloc(sizeof(HIDD));
    if (hidd == NULL)
        return;

    //TODO: find report size, create block
/*    hidd->block = block_create(8);
  */
    hidd->usb = object_get(SYS_OBJ_USB);
    hidd->iface = hid_iface;
    hidd->in_ep = in_ep;
    hidd->in_ep_size = in_ep_size;
    hidd->suspended = false;
    hidd->idle = 0;

#if (USBD_DEBUG_CLASS_REQUESTS)
    printf("Found USB HID device class, EP%d, size: %d, iface: %d\n\r", hidd->in_ep, hidd->in_ep_size, hidd->iface);
#endif //USBD_DEBUG_CLASS_REQUESTS

    size = hidd->in_ep_size;
    fopen_p(hidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | hidd->in_ep), USB_EP_INTERRUPT, (void*)size);
    usbd_register_interface(usbd, hidd->iface, &__HIDD_CLASS, hidd);
    usbd_register_endpoint(usbd, hidd->iface, hidd->in_ep);

}

void hidd_class_reset(USBD* usbd, void* param)
{
    HIDD* hidd = (HIDD*)param;

    fclose(hidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | hidd->in_ep));
    usbd_unregister_endpoint(usbd, hidd->iface, hidd->in_ep);
    usbd_unregister_interface(usbd, hidd->iface, &__HIDD_CLASS);

    hidd_destroy(hidd);
}

void hidd_class_suspend(USBD* usbd, void* param)
{
    HIDD* hidd = (HIDD*)param;
    hidd->suspended = true;
}

void hidd_class_resume(USBD* usbd, void* param)
{
    HIDD* hidd = (HIDD*)param;
    hidd->suspended = false;
}

static inline int hidd_get_report(HIDD* hidd, HANDLE block)
{
    printf("HIDD get report\n\r");
    return -1;
}

static inline int hidd_set_report(HIDD* hidd, HANDLE block)
{
    printf("HIDD set report\n\r");
    return -1;
}

static inline int hidd_get_idle(HIDD* hidd, HANDLE block)
{
#if (USBD_DEBUG_CLASS_REQUESTS)
    printf("HID: get idle\n\r");
#endif
    return -1;
}

static inline int hidd_set_idle(HIDD* hidd, unsigned int value)
{
#if (USBD_DEBUG_CLASS_REQUESTS)
    printf("HID: set idle %dms\n\r", value << 2);
#endif
    //TODO: stop timer here
    hidd->idle = value;
    //TODO: start timer, if required
    return 0;
}

#if (USB_HIDD_BOOT_SUPPORT)
static inline int hidd_get_protocol(HIDD* hidd, HANDLE block)
{
    printf("HIDD get protocol\n\r");
    return -1;
}

static inline int hidd_set_protocol(HIDD* hidd, HANDLE block)
{
    printf("HIDD set protocol\n\r");
    return -1;
}

#endif //USB_HIDD_BOOT_SUPPORT

int hidd_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block)
{
    HIDD* hidd = (HIDD*)param;
    unsigned int res = -1;
    switch (setup->bRequest)
    {
    case HID_GET_REPORT:
        res = hidd_get_report(hidd, block);
        break;
    case HID_GET_IDLE:
        res = hidd_get_idle(hidd, block);
        break;
    case HID_SET_REPORT:
        res = hidd_set_report(hidd, block);
        break;
    case HID_SET_IDLE:
        res = hidd_set_idle(hidd, setup->wValue >> 8);
        break;
#if (USB_HIDD_BOOT_SUPPORT)
    case HID_GET_PROTOCOL:
        res = hidd_get_protocol(hidd, block);
        break;
    case HID_SET_PROTOCOL:
        res = hidd_set_protocol(hidd, block);
        break;
#endif //USB_HIDD_BOOT_SUPPORT
    }
    return res;
}

bool hidd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    printf("HID class reques\n\r");
    return true;
}

const USBD_CLASS __HIDD_CLASS = {
    hidd_class_configured,
    hidd_class_reset,
    hidd_class_suspend,
    hidd_class_resume,
    hidd_class_setup,
    hidd_class_request,
};
