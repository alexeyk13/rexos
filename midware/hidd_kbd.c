/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "hidd_kbd.h"
#include "../userspace/hid.h"
#include "../userspace/stdlib.h"
#include "../userspace/file.h"
#include "sys_config.h"
#include "usbdp.h"

#include "../userspace/stdio.h"
#include "../userspace/block.h"
#include "../userspace/sys.h"
#include <string.h>

//report descriptor
static const uint8_t __KBD_REPORT[HID_BOOT_KEYBOARD_REPORT_SIZE] =
{
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROLS,
    HID_USAGE, HID_USAGE_KEYBOARD,
    HID_COLLECTION, HID_COLLECTION_APPLICATION,
        HID_USAGE_PAGE, HID_USAGE_PAGE_KEYS,
        HID_USAGE_MINIMUM, HID_KEY_LEFT_CONTROL,
        HID_USAGE_MAXIMUM, HID_KEY_RIGHT_GUI,
        HID_LOGICAL_MINIMUM, 0,
        HID_LOGICAL_MAXIMUM, 1,
        HID_REPORT_SIZE, 1,
        HID_REPORT_COUNT, 8,
        HID_INPUT, HID_DATA | HID_VARIABLE | HID_ABSOLUTE,              /* modifier byte */
        HID_REPORT_COUNT, 1,
        HID_REPORT_SIZE, 8,
        HID_INPUT, HID_CONSTANT,                                        /* reserved byte */
        HID_REPORT_COUNT, 5,
        HID_REPORT_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_LEDS,
        HID_USAGE_MINIMUM, HID_LED_NUM_LOCK,
        HID_USAGE_MAXIMUM, HID_LED_CANA,
        HID_OUTPUT, HID_DATA | HID_VARIABLE | HID_ABSOLUTE,             /* LEDs */
        HID_REPORT_COUNT, 1,
        HID_REPORT_SIZE, 3,
        HID_OUTPUT, HID_CONSTANT,                                       /* LEDs padding */
        HID_REPORT_COUNT, 6,
        HID_REPORT_SIZE, 8,
        HID_LOGICAL_MINIMUM, 0,
        HID_LOGICAL_MAXIMUM, 101,
        HID_USAGE_PAGE, HID_USAGE_PAGE_KEYS,
        HID_USAGE_MINIMUM, HID_KEY_NO_PRESS,
        HID_USAGE_MAXIMUM, HID_KEY_APPLICATION,
        HID_INPUT, HID_DATA | HID_ARRAY,                                /* Key arrays (6 bytes) */
    HID_END_COLLECTION
};

typedef struct {
    HANDLE usb;
    HANDLE block;
    BOOT_KEYBOARD kbd;
    uint8_t in_ep, iface;
    uint8_t idle;
    uint8_t suspended, boot_protocol, state, over;
} HIDD_KBD;

static void hidd_kbd_destroy(HIDD_KBD* hidd)
{
    block_destroy(hidd->block);
    free(hidd);
}

static void hidd_kbd_send_report(HIDD_KBD* hidd)
{
    uint8_t* report = block_open(hidd->block);
    if (report == NULL)
        return;
    report[0] = hidd->kbd.modifier;
    report[1] = 0;
    if (hidd->boot_protocol)
    {
        memcpy(report + 2, &hidd->kbd.keys, 6);
        fwrite_async(hidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | hidd->in_ep), hidd->block, 8);
    }
    else
    {
        memcpy(report + 2, &hidd->kbd.leds, sizeof(BOOT_KEYBOARD) - 2);
        fwrite_async(hidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | hidd->in_ep), hidd->block, sizeof(BOOT_KEYBOARD));
    }
}

void hidd_kbd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    uint8_t in_ep, in_ep_size, hid_iface;
    unsigned int size;
    in_ep = in_ep_size = hid_iface = 0;

    //check control/data ep here
    for (iface = usbdp_get_first_interface(cfg); iface != NULL; iface = usbdp_get_next_interface(cfg, iface))
    {
        if (iface->bInterfaceClass == HID_INTERFACE_CLASS && iface->bInterfaceSubClass == HID_SUBCLASS_BOOT_INTERFACE &&
            iface->bInterfaceProtocol == HID_PROTOCOL_KEYBOARD)
        {
            ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usbdp_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX);
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
    HIDD_KBD* hidd = (HIDD_KBD*)malloc(sizeof(HIDD_KBD));
    if (hidd == NULL)
        return;
    hidd->block = block_create(in_ep_size);

    if (hidd->block == INVALID_HANDLE)
    {
        hidd_kbd_destroy(hidd);
        return;
    }
    hidd->usb = object_get(SYS_OBJ_USB);
    hidd->iface = hid_iface;
    hidd->in_ep = in_ep;
    hidd->suspended = false;
    hidd->over = false;
    hidd->idle = 0;
    hidd->boot_protocol = 1;
    hidd->state = USB_HID_KBD_IDLE;
    memset(&hidd->kbd, 0x00, sizeof(BOOT_KEYBOARD));

#if (USBD_HID_DEBUG_REQUESTS)
    printf("Found USB HID device class, EP%d, iface: %d\n\r", hidd->in_ep, hidd->iface);
#endif //USBD_HID_DEBUG_REQUESTS

    size = in_ep_size;
    fopen_p(hidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | hidd->in_ep), USB_EP_INTERRUPT, (void*)size);
    usbd_register_interface(usbd, hidd->iface, &__HIDD_KBD_CLASS, hidd);
    usbd_register_endpoint(usbd, hidd->iface, hidd->in_ep);

}

void hidd_kbd_class_reset(USBD* usbd, void* param)
{
    HIDD_KBD* hidd = (HIDD_KBD*)param;

    fclose(hidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | hidd->in_ep));
    usbd_unregister_endpoint(usbd, hidd->iface, hidd->in_ep);
    usbd_unregister_interface(usbd, hidd->iface, &__HIDD_KBD_CLASS);

    hidd_kbd_destroy(hidd);
}

void hidd_kbd_class_suspend(USBD* usbd, void* param)
{
    HIDD_KBD* hidd = (HIDD_KBD*)param;
    fflush(hidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | hidd->in_ep));
    hidd->state = USB_HID_KBD_IDLE;
    hidd->suspended = true;
}

void hidd_kbd_class_resume(USBD* usbd, void* param)
{
    HIDD_KBD* hidd = (HIDD_KBD*)param;
    hidd->suspended = false;
}

static inline int hidd_kbd_get_descriptor(HIDD_KBD* hidd, unsigned int value, unsigned int index, HANDLE block)
{
    int res = -1;
    uint8_t* descriptor = block_open(block);
    if (descriptor == NULL)
        return -1;
    switch (value)
    {
    case HID_REPORT_DESCRIPTOR_TYPE:
#if (USBD_HID_DEBUG_REQUESTS)
        printf("HIDD KBD: get REPORT DESCRIPTOR\n\r");
#endif
        memcpy(descriptor, &__KBD_REPORT, HID_BOOT_KEYBOARD_REPORT_SIZE);
        res = HID_BOOT_KEYBOARD_REPORT_SIZE;
        break;
    }
    return res;
}

static inline int hidd_kbd_get_report(HIDD_KBD* hidd, unsigned int type, HANDLE block)
{
    int res = -1;
    uint8_t* report = block_open(block);
    if (report == NULL)
        return -1;
    switch (type)
    {
    case HID_REPORT_TYPE_INPUT:
#if (USBD_HID_DEBUG_REQUESTS)
        printf("HIDD KBD: get INPUT report\n\r");
#endif
        report[0] = hidd->kbd.modifier;
        report[1] = 0;
        memcpy(report + 2, &hidd->kbd.keys, 6);
        res = 8;
        break;
    case HID_REPORT_TYPE_OUTPUT:
#if (USBD_HID_DEBUG_REQUESTS)
        printf("HIDD KBD: get OUTPUT report\n\r");
#endif
        report[0] = hidd->kbd.leds;
        res = 1;
        break;
    }
    return res;
}

static inline int hidd_kbd_set_report(USBD* usbd, HIDD_KBD* hidd, HANDLE block, unsigned int length)
{
    uint8_t* report = block_open(block);
    if (report == NULL)
        return -1;
#if (USBD_HID_DEBUG_REQUESTS)
    printf("HIDD KBD: set LEDs %#X\n\r", report[0]);
#endif
    if (hidd->kbd.leds != report[0])
        usbd_post_user(usbd, hidd->iface, USB_HID_KBD_LEDS_STATE_CHANGED, report[0]);
    hidd->kbd.leds = report[0];
    return 0;
}

static inline int hidd_kbd_get_idle(HIDD_KBD* hidd, HANDLE block)
{
#if (USBD_HID_DEBUG_REQUESTS)
    printf("HIDD KBD: get idle\n\r");
#endif
    uint8_t* ptr = block_open(block);
    if (ptr == NULL)
        return -1;
    ptr[0] = hidd->idle;
    return 1;
}

static inline int hidd_kbd_set_idle(HIDD_KBD* hidd, unsigned int value)
{
#if (USBD_HID_DEBUG_REQUESTS)
    printf("HIDD KBD: set idle %dms\n\r", value << 2);
#endif
    //no IDLE report is supported
    return 0;
}

static inline int hidd_kbd_get_protocol(HIDD_KBD* hidd, HANDLE block)
{
#if (USBD_HID_DEBUG_REQUESTS)
    printf("HIDD KBD: get protocol\n\r");
#endif
    uint8_t* ptr = block_open(block);
    if (ptr == NULL)
        return -1;
    ptr[0] = hidd->boot_protocol;
    return 1;
}

static inline int hidd_kbd_set_protocol(HIDD_KBD* hidd, unsigned int protocol, HANDLE block)
{
#if (USBD_HID_DEBUG_REQUESTS)
    printf("HIDD KBD: set protocol: %d\n\r", protocol & 1);
#endif
    hidd->boot_protocol = protocol & 1;
    return 0;
}

int hidd_kbd_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block)
{
    HIDD_KBD* hidd = (HIDD_KBD*)param;
    unsigned int res = -1;
    switch (setup->bRequest)
    {
    case USB_REQUEST_GET_DESCRIPTOR:
        res = hidd_kbd_get_descriptor(hidd, setup->wValue >> 8, setup->wValue & 0xff, block);
        break;
    case HID_GET_REPORT:
        res = hidd_kbd_get_report(hidd, setup->wValue >> 8, block);
        break;
    case HID_GET_IDLE:
        res = hidd_kbd_get_idle(hidd, block);
        break;
    case HID_SET_REPORT:
        res = hidd_kbd_set_report(usbd, hidd, block, setup->wLength);
        break;
    case HID_SET_IDLE:
        res = hidd_kbd_set_idle(hidd, setup->wValue >> 8);
        break;
    case HID_GET_PROTOCOL:
        res = hidd_kbd_get_protocol(hidd, block);
        break;
    case HID_SET_PROTOCOL:
        res = hidd_kbd_set_protocol(hidd, setup->wValue, block);
        break;
    }
    return res;
}

static inline void hidd_kbd_write_complete(USBD* usbd, HIDD_KBD* hidd)
{
    usbd_post_user(usbd, hidd->iface, hidd->state, 0);
    hidd->state = USB_HID_KBD_IDLE;
}

static inline void hidd_kbd_modifier_change(USBD* usbd, HIDD_KBD* hidd, uint8_t modifier)
{
    if (hidd->state != USB_HID_KBD_IDLE)
    {
        error(ERROR_IN_PROGRESS);
        usbd_post_user(usbd, hidd->iface, USB_HID_KBD_MODIFIER_CHANGE, 0);
        return;
    }
    hidd->kbd.modifier = modifier;
    hidd->state = USB_HID_KBD_MODIFIER_CHANGE;
    hidd_kbd_send_report(hidd);
}

static inline void hidd_kbd_key_press(USBD* usbd, HIDD_KBD* hidd, unsigned int key)
{
    unsigned int swap, tmp;
    int i;
    if (hidd->state != USB_HID_KBD_IDLE)
    {
        error(ERROR_IN_PROGRESS);
        usbd_post_user(usbd, hidd->iface, USB_HID_KBD_KEY_PRESS, 0);
        return;
    }
    for (i = 0; i < 6 && hidd->kbd.keys[i]; ++i)
        if (hidd->kbd.keys[i] == key)
        {
            error(ERROR_ALREADY_CONFIGURED);
            usbd_post_user(usbd, hidd->iface, USB_HID_KBD_KEY_PRESS, 0);
            return;
        }

    if (hidd->kbd.keys[5])
        hidd->over = true;
    else
    {
        for (i = 0; i < 6; ++i)
        {
            if (hidd->kbd.keys[i] == 0 || hidd->kbd.keys[i] > key)
            {
                swap = hidd->kbd.keys[i];
                hidd->kbd.keys[i] = key;
                for (++i; swap && i < 6; ++i)
                {
                    tmp = hidd->kbd.keys[i];
                    hidd->kbd.keys[i] = swap;
                    swap = tmp;
                }
                break;
            }
        }
    }
    hidd->state = USB_HID_KBD_KEY_PRESS;
    hidd_kbd_send_report(hidd);
}

static inline void hidd_kbd_key_release(USBD* usbd, HIDD_KBD* hidd, unsigned int key)
{
    int i;
    if (hidd->state != USB_HID_KBD_IDLE)
    {
        error(ERROR_IN_PROGRESS);
        usbd_post_user(usbd, hidd->iface, USB_HID_KBD_KEY_RELEASE, 0);
        return;
    }
    hidd->over = false;
    for (i = 0; i < 6 && hidd->kbd.keys[i]; ++i)
    {
        if (hidd->kbd.keys[i] == key)
        {
            for (;i < 5 && hidd->kbd.keys[i + 1]; ++i)
            {
                hidd->kbd.keys[i] = hidd->kbd.keys[i + 1];
            }
            hidd->kbd.keys[i] = 0;
            break;
        }
    }
    hidd->state = USB_HID_KBD_KEY_RELEASE;
    hidd_kbd_send_report(hidd);
}

static inline void hidd_kbd_get_leds_state(HIDD_KBD* hidd, IPC* ipc)
{
    ipc->param2 = hidd->kbd.leds;
}

bool hidd_kbd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    HIDD_KBD* hidd = (HIDD_KBD*)param;
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_WRITE:
        hidd_kbd_write_complete(usbd, hidd);
        break;
    case USBD_INTERFACE_REQUEST:
        switch (ipc->param2)
        {
        case USB_HID_KBD_MODIFIER_CHANGE:
            hidd_kbd_modifier_change(usbd, hidd, ipc->param3);
            break;
        case USB_HID_KBD_KEY_PRESS:
            hidd_kbd_key_press(usbd, hidd, ipc->param3);
            break;
        case USB_HID_KBD_KEY_RELEASE:
            hidd_kbd_key_release(usbd, hidd, ipc->param3);
            break;
        case USB_HID_KBD_GET_LEDS_STATE:
            hidd_kbd_get_leds_state(hidd, ipc);
            need_post = true;
            break;
        }
        break;
    }
    return need_post;
}

const USBD_CLASS __HIDD_KBD_CLASS = {
    hidd_kbd_class_configured,
    hidd_kbd_class_reset,
    hidd_kbd_class_suspend,
    hidd_kbd_class_resume,
    hidd_kbd_class_setup,
    hidd_kbd_class_request,
};
