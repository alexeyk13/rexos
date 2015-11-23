/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "usbd.h"
#include "../../userspace/sys.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stdlib.h"
#include <string.h>
#include "../../userspace/array.h"
#if (USBD_CDC_ACM_CLASS)
#include "cdc_acmd.h"
#endif //USBD_CDC_ACM_CLASS
#if (USBD_HID_KBD_CLASS)
#include "hidd_kbd.h"
#endif //USBD_HID_KBD_CLASS
#if (USBD_MSC_CLASS)
#include "mscd.h"
#endif //USBD_MSC_CLASS
#if (USBD_CCID_CLASS)
#include "ccidd.h"
#endif //USBD_CCID_CLASS

typedef enum {
    USB_SETUP_STATE_REQUEST = 0,
    USB_SETUP_STATE_VENDOR_REQUEST,
    USB_SETUP_STATE_DATA_IN,
    //in case response is less, than request
    USB_SETUP_STATE_DATA_IN_ZLP,
    USB_SETUP_STATE_DATA_OUT,
    USB_SETUP_STATE_STATUS_IN,
    USB_SETUP_STATE_STATUS_OUT
} USB_SETUP_STATE;

typedef struct {
    uint16_t index, lang;
    USB_DESCRIPTOR_TYPE** d;
} USBD_DESCRIPTOR;

typedef enum {
    USBD_STATE_DEFAULT = 0,
    USBD_STATE_ADDRESSED,
    USBD_STATE_CONFIGURED
} USBD_STATE;

typedef struct _USBD {
    HANDLE usb, user;
    USB_PORT_TYPE port;
    IO* io;
    //SETUP state machine
    SETUP setup;
    USB_SETUP_STATE setup_state;
    // USBD state machine
    USBD_STATE state;
    bool self_powered, remote_wakeup, suspended;
#if (USB_TEST_MODE_SUPPORT)
    USB_TEST_MODES test_mode;
#endif //USB_TEST_MODE_SUPPORT
    USB_SPEED speed;
    uint8_t ep0_size;
    uint8_t configuration;
    ARRAY *descriptors;
    ARRAY *ifaces;
    uint8_t ep_iface[USB_EP_COUNT_MAX];
    uint8_t ifacecnt;
} USBD;

typedef struct {
    const USBD_CLASS* usbd_class;
    void* param;
} USBD_IFACE_ENTRY;

#define IFACE(usbd, iface)                          ((USBD_IFACE_ENTRY*)array_at((usbd)->ifaces, iface))
#define DESCRIPTOR(usbd, i)                         ((USBD_DESCRIPTOR*)array_at((usbd)->descriptors, i))

#define USBD_INVALID_INTERFACE                      0xff

void usbd_stub_class_state_change(USBD* usbd, void* param);
int usbd_stub_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io);
void usbd_stub_class_request(USBD* usbd, void* param, IPC* ipc);

const USBD_CLASS __USBD_STUB_CLASS = {
    NULL,
    usbd_stub_class_state_change,
    usbd_stub_class_state_change,
    usbd_stub_class_state_change,
    usbd_stub_class_setup,
    usbd_stub_class_request,
};

static const USBD_CLASS* __USBD_CLASSES[] =         {
#if (USBD_CDC_ACM_CLASS)
                                                        &__CDC_ACMD_CLASS,
#endif //USBD_CDC_ACM_CLASS
#if (USBD_HID_KBD_CLASS)
                                                        &__HIDD_KBD_CLASS,
#endif //USBD_HID_KBD_CLASS
#if (USBD_MSC_CLASS)
                                                        &__MSCD_CLASS,
#endif //USBD_MSC_CLASS
#if (USBD_CCID_CLASS)
                                                        &__CCIDD_CLASS,
#endif //USBD_CCID_CLASS
                                                        &__USBD_STUB_CLASS
                                                    };
HANDLE usbd_user(USBD* usbd)
{
    return usbd->user;
}

HANDLE usbd_usb(USBD* usbd)
{
    return usbd->usb;
}

void usbd_post_user(USBD* usbd, unsigned int iface, unsigned int num, unsigned int cmd, unsigned int param2, unsigned int param3)
{
    if (usbd->user == INVALID_HANDLE)
        return;
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = usbd->user;
    ipc.param1 = USBD_IFACE(iface, num);
    ipc.param2 = param2;
    ipc.param3 = param3;
    ipc_post(&ipc);
}

void usbd_io_user(USBD* usbd, unsigned int iface, unsigned int num, unsigned int cmd, IO* io, unsigned int param3)
{
    if (usbd->user == INVALID_HANDLE)
        return;
    ipc_post_inline(usbd->user, cmd, USBD_IFACE(iface, num), (unsigned int)io, param3);
}

bool usbd_register_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class, void* param)
{
    if (iface >= usbd->ifacecnt)
        return false;
    if (IFACE(usbd, iface)->usbd_class != &__USBD_STUB_CLASS)
        return false;
    IFACE(usbd, iface)->usbd_class = usbd_class;
    IFACE(usbd, iface)->param = param;
    return true;
}

bool usbd_unregister_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class)
{
    if (iface >= usbd->ifacecnt)
        return false;
    if (IFACE(usbd, iface)->usbd_class != usbd_class)
        return false;
    IFACE(usbd, iface)->usbd_class = &__USBD_STUB_CLASS;
    return true;
}

bool usbd_register_endpoint(USBD* usbd, unsigned int iface, unsigned int ep_num)
{
    if (iface >= usbd->ifacecnt || ep_num >= USB_EP_COUNT_MAX)
        return false;
    if (IFACE(usbd, iface)->usbd_class == &__USBD_STUB_CLASS)
        return false;
    if (usbd->ep_iface[ep_num] != USBD_INVALID_INTERFACE)
        return false;
    usbd->ep_iface[ep_num] = iface;
    return true;
}

bool usbd_unregister_endpoint(USBD* usbd, unsigned int iface, unsigned int ep_num)
{
    if (iface >= usbd->ifacecnt || ep_num >= USB_EP_COUNT_MAX)
        return false;
    if (IFACE(usbd, iface)->usbd_class == &__USBD_STUB_CLASS)
        return false;
    if (usbd->ep_iface[ep_num] != iface)
        return false;
    usbd->ep_iface[ep_num] = USBD_INVALID_INTERFACE;
    return true;
}

void usbd_usb_ep_open(USBD* usbd, unsigned int num, USB_EP_TYPE type, unsigned int size)
{
    ack(usbd->usb, HAL_REQ(HAL_USB, IPC_OPEN), USB_HANDLE(usbd->port, num), type, size);
}

void usbd_usb_ep_close(USBD* usbd, unsigned int num)
{
    ack(usbd->usb, HAL_REQ(HAL_USB, IPC_CLOSE), USB_HANDLE(usbd->port, num), 0, 0);
}

void usbd_usb_ep_flush(USBD* usbd, unsigned int num)
{
    ack(usbd->usb, HAL_REQ(HAL_USB, IPC_FLUSH), USB_HANDLE(usbd->port, num), 0, 0);
}

void usbd_usb_ep_set_stall(USBD* usbd, unsigned int num)
{
    ack(usbd->usb, HAL_REQ(HAL_USB, USB_EP_SET_STALL), USB_HANDLE(usbd->port, num), 0, 0);
}

void usbd_usb_ep_clear_stall(USBD* usbd, unsigned int num)
{
    ack(usbd->usb, HAL_REQ(HAL_USB, USB_EP_CLEAR_STALL), USB_HANDLE(usbd->port, num), 0, 0);
}

void usbd_usb_ep_write(USBD* usbd, unsigned int ep_num, IO* io)
{
    io_write(usbd->usb, HAL_IO_REQ(HAL_USB, IPC_WRITE), USB_HANDLE(usbd->port, USB_EP_IN | ep_num), io);
}

void usbd_usb_ep_read(USBD* usbd, unsigned int ep_num, IO* io, unsigned int size)
{
    io_read(usbd->usb, HAL_IO_REQ(HAL_USB, IPC_READ), USB_HANDLE(usbd->port, ep_num), io, size);
}

static int usbd_get_descriptor_index(USBD* usbd, unsigned int type, unsigned int index, unsigned int lang)
{
    int i;
    for (i = 0; i < array_size(usbd->descriptors); ++i)
    {
        if (DESCRIPTOR(usbd, i)->index == index && DESCRIPTOR(usbd, i)->lang == lang &&
            (*(DESCRIPTOR(usbd, i)->d))->bDescriptorType == type)
            return i;
    }
    return -1;
}

static USB_DESCRIPTOR_TYPE* usbd_descriptor(USBD* usbd, unsigned int type, unsigned int index, unsigned int lang)
{
    int idx = usbd_get_descriptor_index(usbd, type, index, lang);
    if (idx < 0)
        return NULL;
    return *(DESCRIPTOR(usbd, idx)->d);
}

void usbd_stub_class_state_change(USBD* usbd, void* param)
{
    //state changing request stub calling is not error in case of some interfaces are not used
    //Moreover, it's normal condition to avoid MS Windows bug for composite devices, where interfaces are starting from 1
}

int usbd_stub_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io)
{
#if (USBD_DEBUG_ERRORS)
    printf("USBD class SETUP stub!\n");
#endif
    return -1;
}

void usbd_stub_class_request(USBD* usbd, void* param, IPC* ipc)
{
#if (USBD_DEBUG_ERRORS)
    printf("USBD class request stub!\n");
#endif
}

static void usbd_inform(USBD* usbd, unsigned int alert)
{
    if (usbd->user != INVALID_HANDLE)
        ipc_post_inline(usbd->user, HAL_CMD(HAL_USBD, USBD_ALERT), alert, 0, 0);
}

static void usbd_class_reset(USBD* usbd)
{
    int i;
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class->usbd_class_reset(usbd, IFACE(usbd, i)->param);
    usbd->ifacecnt = 0;
    array_clear(&usbd->ifaces);
}

static inline void usbd_class_suspend(USBD* usbd)
{
    int i;
    usbd_inform(usbd, USBD_ALERT_SUSPEND);
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class->usbd_class_suspend(usbd, IFACE(usbd, i)->param);
}

static inline void usbd_class_resume(USBD* usbd)
{
    int i;
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class->usbd_class_resume(usbd, IFACE(usbd, i)->param);
    usbd_inform(usbd, USBD_ALERT_RESUME);
}

void usbd_fatal(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
        usbd->state = USBD_STATE_DEFAULT;
        usbd_inform(usbd, USBD_ALERT_RESET);
        usbd_class_reset(usbd);
    }
    usbd_usb_ep_set_stall(usbd, 0);
    usbd_usb_ep_set_stall(usbd, USB_EP_IN | 0);
}

void usbd_fatal_stub(USBD* usbd)
{
#if (USB_DEBUG_ERRORS)
    printf("USBD fatal: stub called\n");
#endif
    usbd_fatal(usbd);
}

static inline USB_CONFIGURATION_DESCRIPTOR_TYPE* usbd_configuration_descriptor(USBD* usbd)
{
    int i;
    USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg = NULL;
    USB_DEVICE_DESCRIPTOR_TYPE* device = (USB_DEVICE_DESCRIPTOR_TYPE*)usbd_descriptor(usbd,
                                           (usbd->speed < USB_LOW_SPEED_ALT) ?
                                           USB_DEVICE_DESCRIPTOR_INDEX : USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, 0, 0);
    if (device)
        for (i = 0; i < device->bNumConfigurations; ++i)
        {
            cfg = (USB_CONFIGURATION_DESCRIPTOR_TYPE*)usbd_descriptor(usbd,
                   (usbd->speed < USB_LOW_SPEED_ALT) ?
                   USB_CONFIGURATION_DESCRIPTOR_INDEX : USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX, i, 0);
            if (!cfg)
                break;
            if (cfg->bConfigurationValue == usbd->configuration)
                break;
        }
    return cfg;
}


static inline void usbd_class_configured(USBD* usbd)
{
    USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg;
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;

    int i;
    cfg = usbd_configuration_descriptor(usbd);
    if (cfg == NULL)
    {
#if (USBD_DEBUG_ERRORS)
        printf("USBD fatal: No selected configuration (%d) found\n", usbd->configuration);
#endif
        usbd_fatal(usbd);
        return;
    }
    //find num of interfaces in configuration
    for (iface = usb_get_first_interface(cfg), usbd->ifacecnt = 0; iface != NULL; iface = usb_get_next_interface(cfg, iface))
        if (iface->bInterfaceNumber >= usbd->ifacecnt)
            usbd->ifacecnt = iface->bInterfaceNumber + 1;
    for (i = 0; i <= usbd->ifacecnt; ++i)
        array_append(&usbd->ifaces);
    if (usbd->ifaces == NULL)
    {
#if (USBD_DEBUG_ERRORS)
        printf("USBD fatal: Out of memory\n");
#endif
        process_exit();
        return;
    }
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class = &__USBD_STUB_CLASS;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
        usbd->ep_iface[i] = USBD_INVALID_INTERFACE;

    //check all classes for interface
    for (i = 0; __USBD_CLASSES[i] != &__USBD_STUB_CLASS; ++i)
        __USBD_CLASSES[i]->usbd_class_configured(usbd, cfg);

    usbd_inform(usbd, USBD_ALERT_CONFIGURED);
}

static inline void usbd_open(USBD* usbd, USB_PORT_TYPE port)
{
    if (usbd->io != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    usbd->io = io_create(USBD_IO_SIZE);
    if (usbd->io == NULL)
        return;
    usbd->setup_state = USB_SETUP_STATE_REQUEST;
    usbd->state = USBD_STATE_DEFAULT;
    usbd->self_powered = usbd->remote_wakeup = false;
#if (USB_TEST_MODES)
    usbd->test_mode = USB_TEST_MODE_NORMAL;
#endif //USB_TEST_MODES

    usbd->speed = USB_LOW_SPEED;
    usbd->ep0_size = 0;
    usbd->configuration = 0;
    usbd->port = port;
    ack(usbd->usb, HAL_REQ(HAL_USB, IPC_OPEN), USB_HANDLE(usbd->port, USB_HANDLE_DEVICE), 0, 0);
}

static inline void usbd_close(USBD* usbd)
{
    if (usbd->io == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (usbd->state == USBD_STATE_CONFIGURED)
        usbd_class_reset(usbd);
    if (usbd->ep0_size)
    {
        usbd_usb_ep_close(usbd, 0);
        usbd_usb_ep_close(usbd, USB_EP_IN | 0);
        usbd->ep0_size = 0;
    }

    ack(usbd->usb, HAL_REQ(HAL_USB, IPC_CLOSE), USB_HANDLE(usbd->port, USB_HANDLE_DEVICE), 0, 0);
    io_destroy(usbd->io);
    usbd->io = NULL;
}

static inline int usbd_request_register_descriptor(USBD* usbd, IO* io)
{
    int idx;
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs = io_stack(io);
    if (usbd_get_descriptor_index(usbd, ((USB_DESCRIPTOR_TYPE*)io_data(io))->bDescriptorType, udrs->index, udrs->lang) >= 0)
        return ERROR_ALREADY_CONFIGURED;

    void* d = malloc(io->data_size);
    if (d == NULL)
        return get_last_error();
    memcpy(d, io_data(io), io->data_size);
    if (*((void**)d) == NULL)
        *((void**)d) = d + sizeof(void*);
    array_append(&usbd->descriptors);
    idx = array_size(usbd->descriptors) - 1;

    DESCRIPTOR(usbd, idx)->index = udrs->index;
    DESCRIPTOR(usbd, idx)->lang = udrs->lang;
    DESCRIPTOR(usbd, idx)->d = d;
    return idx;
}

static inline void usbd_request_unregister_descriptor(USBD* usbd, int idx)
{
    free(DESCRIPTOR(usbd, idx)->d);
    array_remove(&usbd->descriptors, idx);
}

static inline void usbd_reset(USBD* usbd, USB_SPEED speed)
{
    //don't reset on configured state to be functional in virtualbox environment
    if (usbd->state != USBD_STATE_CONFIGURED)
        usbd->state = USBD_STATE_DEFAULT;
#if (USBD_DEBUG_REQUESTS)
    printf("USB device reset\n");
#endif
    if (usbd->ep0_size)
    {
        usbd_usb_ep_close(usbd, 0);
        usbd_usb_ep_close(usbd, USB_EP_IN | 0);
    }

    usbd->speed = speed;
    usbd->ep0_size = usbd->speed == USB_LOW_SPEED ? 8 : 64;

    usbd_usb_ep_open(usbd, 0, USB_EP_CONTROL, usbd->ep0_size);
    usbd_usb_ep_open(usbd, USB_EP_IN | 0, USB_EP_CONTROL, usbd->ep0_size);

    usbd->setup_state = USB_SETUP_STATE_REQUEST;
    usbd->suspended = false;
}

static inline void usbd_suspend(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
#if (USBD_DEBUG_REQUESTS)
        printf("USB device suspend\n");
#endif
        usbd_class_suspend(usbd);
        if (usbd->ep0_size)
        {
            usbd_usb_ep_close(usbd, 0);
            usbd_usb_ep_close(usbd, USB_EP_IN | 0);
        }
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        usbd->suspended = true;
    }
}

static inline void usbd_wakeup(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
#if (USBD_DEBUG_REQUESTS)
        printf("USB device wakeup\n");
#endif
        usbd_usb_ep_open(usbd, 0, USB_EP_CONTROL, usbd->ep0_size);
        usbd_usb_ep_open(usbd, USB_EP_IN | 0, USB_EP_CONTROL, usbd->ep0_size);

        usbd_class_resume(usbd);
        usbd->suspended = false;
    }
}

static inline int usbd_device_get_status(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: get device status\n");
#endif
    uint16_t status = 0;
    if (usbd->self_powered)
        status |= 1 << 0;
    if (usbd->remote_wakeup)
        status |= 1 << 1;
    return io_data_write(usbd->io, &status, sizeof(uint16_t));
}

static inline int usbd_device_set_feature(USBD* usbd)
{
    unsigned int res = -1;
    //According to documentation the only feature can be set is TEST_MODE
    //However, linux is issuing feature REMOTE_WAKEUP, so we must handle it to proper
    //remote wakeup
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_DEVICE_REMOTE_WAKEUP:
#if (USBD_DEBUG_REQUESTS)
        printf("USB: device set feature REMOTE WAKEUP\n");
#endif
        usbd->remote_wakeup = true;
        res = 0;
        break;
#if (USB_TEST_MODE_SUPPORT)
    case USBD_FEATURE_TEST_MODE:
#if (USBD_DEBUG_REQUESTS)
        printf("USB: device set feature TEST_MODE\n");
#endif
        usbd->test_mode = usbd->setup.wIndex >> 16;
        ack(usbd->usb, HAL_REQ(HAL_USB, USB_SET_TEST_MODE), USB_HANDLE(usbd->port, USB_HANDLE_DEVICE), usbd->test_mode, 0);
        res = 0;
        break;
#endif //USB_TEST_MODE_SUPPORT
    default:
        break;
    }
    return res;
}

static inline int usbd_device_clear_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USBD_DEBUG_REQUESTS)
    printf("USB: device clear feature\n");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_DEVICE_REMOTE_WAKEUP:
        usbd->remote_wakeup = false;
        res = 0;
        break;
    default:
        break;
    }
    return res;
}

static inline int usbd_set_address(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB set ADDRESS %#X\n", usbd->setup.wValue);
#endif
    ack(usbd->usb, HAL_REQ(HAL_USB, USB_SET_ADDRESS), USB_HANDLE(usbd->port, USB_HANDLE_DEVICE), usbd->setup.wValue, 0);
    switch (usbd->state)
    {
    case USBD_STATE_DEFAULT:
        usbd->state = USBD_STATE_ADDRESSED;
        break;
    case USBD_STATE_ADDRESSED:
        if (usbd->setup.wValue == 0)
            usbd->state = USBD_STATE_DEFAULT;
        break;
    default:
        //some hosts doesn't issuing reset on reset request
        usbd->configuration = 0;

        usbd->state = USBD_STATE_ADDRESSED;
        usbd_inform(usbd, USBD_ALERT_RESET);
        usbd_class_reset(usbd);
        break;
    }
    return 0;
}

static int send_descriptor(USBD* usbd, unsigned int type, unsigned int index, unsigned int lang, unsigned int send_type)
{
    unsigned int size;
    USB_DESCRIPTOR_TYPE* d = usbd_descriptor(usbd, type, index, lang);
    if (d == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return -1;
    }
    if (type == USB_CONFIGURATION_DESCRIPTOR_INDEX || type == USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX)
        size = ((USB_CONFIGURATION_DESCRIPTOR_TYPE*)d)->wTotalLength;
    else
        size = d->bLength;

    size = io_data_write(usbd->io, d, size);
    ((USB_DESCRIPTOR_TYPE*)io_data(usbd->io))->bDescriptorType = send_type;
    return size;
}

static inline int usbd_get_descriptor(USBD* usbd)
{
    //can be called in any device state
    int res = -1;

    unsigned int type = usbd->setup.wValue >> 8;
    unsigned int index = usbd->setup.wValue & 0xff;
    unsigned int lang = usbd->setup.wIndex;
    switch (type)
    {
    case USB_DEVICE_DESCRIPTOR_INDEX:
        if (usbd->speed < USB_LOW_SPEED_ALT)
            res = send_descriptor(usbd, USB_DEVICE_DESCRIPTOR_INDEX, 0, 0, USB_DEVICE_DESCRIPTOR_INDEX);
        else
            res = send_descriptor(usbd, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, 0, 0, USB_DEVICE_DESCRIPTOR_INDEX);
#if (USBD_DEBUG_REQUESTS)
        printf("USB get DEVICE descriptor\n");
#endif
        break;
    case USB_CONFIGURATION_DESCRIPTOR_INDEX:
        if (usbd->speed < USB_LOW_SPEED_ALT)
            res = send_descriptor(usbd, USB_CONFIGURATION_DESCRIPTOR_INDEX, index, 0, USB_CONFIGURATION_DESCRIPTOR_INDEX);
        else
            res = send_descriptor(usbd, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX, index, 0, USB_CONFIGURATION_DESCRIPTOR_INDEX);
#if (USBD_DEBUG_REQUESTS)
        printf("USB get CONFIGURATION %d descriptor\n", index);
#endif
        break;
    case USB_STRING_DESCRIPTOR_INDEX:
        res = send_descriptor(usbd, USB_STRING_DESCRIPTOR_INDEX, index, lang, USB_STRING_DESCRIPTOR_INDEX);
#if (USBD_DEBUG_REQUESTS)
        printf("USB get STRING %d descriptor, lang: %#X\n", index, lang);
#endif
        break;
    case USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX:
        if (usbd->speed < USB_LOW_SPEED_ALT)
            res = send_descriptor(usbd, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, 0, 0, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX);
        if (res < 0)
            res = send_descriptor(usbd, USB_DEVICE_DESCRIPTOR_INDEX, 0, 0, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX);
#if (USBD_DEBUG_REQUESTS)
        printf("USB get DEVICE qualifier descriptor\n");
#endif
        break;
    case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX:
        if (usbd->speed < USB_LOW_SPEED_ALT)
            res = send_descriptor(usbd, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX, index, 0, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
        if (res < 0)
            res = send_descriptor(usbd, USB_CONFIGURATION_DESCRIPTOR_INDEX, index, 0, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
#if (USBD_DEBUG_REQUESTS)
        printf("USB get other speed CONFIGURATION %d descriptor\n", index);
#endif
    }

#if (USBD_DEBUG_ERRORS)
    if (res < 0)
        printf("USB: descriptor type %d, index: %d, lang: %#X not present\n", type, index, lang);
#endif

    return res;
}

static inline int usbd_get_configuration(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: get configuration\n");
#endif
    char configuration = (char)usbd->configuration;
    return io_data_write(usbd->io, &configuration, 1);
}

static inline int usbd_set_configuration(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: set configuration %d\n", usbd->setup.wValue);
#endif
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
        usbd->configuration = 0;

        usbd->state = USBD_STATE_ADDRESSED;
        usbd_inform(usbd, USBD_ALERT_RESET);
        usbd_class_reset(usbd);
    }
    //USB 2.0 specification is required to set USB state addressed if it was already configured
    //However, due to virtualbox bug - if device is switching connection between VMs, it's issuing
    //SET CONFIGURATION without prior reset, causing logical reset, so this feature is disabled
    if (usbd->state == USBD_STATE_ADDRESSED && usbd->setup.wValue)
    {
        usbd->configuration = usbd->setup.wValue;
        usbd->state = USBD_STATE_CONFIGURED;

        usbd_class_configured(usbd);
    }
    return 0;
}

static inline int usbd_standart_device_request(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
        res = usbd_device_get_status(usbd);
        break;
    case USB_REQUEST_SET_FEATURE:
        res = usbd_device_set_feature(usbd);
        break;
    case USB_REQUEST_CLEAR_FEATURE:
        res = usbd_device_clear_feature(usbd);
        break;
    case USB_REQUEST_SET_ADDRESS:
        res = usbd_set_address(usbd);
        break;
    case USB_REQUEST_GET_DESCRIPTOR:
        res = usbd_get_descriptor(usbd);
        break;
    case USB_REQUEST_GET_CONFIGURATION:
        res = usbd_get_configuration(usbd);
        break;
    case USB_REQUEST_SET_CONFIGURATION:
        res = usbd_set_configuration(usbd);
        break;
    }
    return res;
}

static inline int usbd_endpoint_get_status(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: get endpoint status\n");
#endif
    uint16_t status = 0;
    if (get(usbd->usb, HAL_REQ(HAL_USB, USB_EP_IS_STALL), USB_HANDLE(usbd->port, usbd->setup.wIndex), 0, 0))
        status |= 1 << 0;
    return io_data_write(usbd->io, &status, sizeof(uint16_t));
}

static inline int usbd_endpoint_set_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USBD_DEBUG_REQUESTS)
    printf("USB: endpoint set feature\n");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        usbd_usb_ep_set_stall(usbd, usbd->setup.wIndex);
        res = 0;
        break;
    default:
        break;
    }
    return res;
}

static inline int usbd_endpoint_clear_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USBD_DEBUG_REQUESTS)
    printf("USB: endpoint clear feature\n");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        usbd_usb_ep_clear_stall(usbd, usbd->setup.wIndex);
        res = 0;
        break;
    default:
        break;
    }
    return res;
}

static inline int usbd_interface_request(USBD* usbd, int iface)
{
    int res = -1;
    if (iface < usbd->ifacecnt)
        res = IFACE(usbd, iface)->usbd_class->usbd_class_setup(usbd, IFACE(usbd, iface)->param, &usbd->setup, usbd->io);
#if (USBD_DEBUG_ERRORS)
    else
        printf("USBD: Interface %d is not configured\n");
#endif
    return res;
}

static inline int usbd_endpoint_request(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
        res = usbd_endpoint_get_status(usbd);
        break;
    case USB_REQUEST_SET_FEATURE:
        res = usbd_endpoint_set_feature(usbd);
        break;
    case USB_REQUEST_CLEAR_FEATURE:
        res = usbd_endpoint_clear_feature(usbd);
        break;
    }
    return res;
}

static void usbd_setup_response(USBD* usbd, int res)
{
    if (res > usbd->setup.wLength)
        res = usbd->setup.wLength;
    //success. start transfers
    if (res >= 0)
    {
        usbd->io->data_size = res;
        if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
        {
            //data already received, sending status
            usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
#if (USBD_DEBUG_FLOW)
            printf("STATUS IN\n");
#endif //USBD_DEBUG_FLOW
            usbd_usb_ep_write(usbd, 0, usbd->io);
        }
        else
        {
            //response less, than required and multiples of EP0SIZE - we need to send ZLP on end of transfers
            if (res < usbd->setup.wLength && ((res % usbd->ep0_size) == 0))
            {
                if (res)
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN_ZLP;
                else
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                }
#if (USBD_DEBUG_FLOW)
                printf("DATA IN: %d\n", res);
#endif //USBD_DEBUG_FLOW
                usbd_usb_ep_write(usbd, 0, usbd->io);
            }
            else if (res)
            {
                usbd->setup_state = USB_SETUP_STATE_DATA_IN;
#if (USBD_DEBUG_FLOW)
                printf("DATA IN: %d\n", res);
#endif //USBD_DEBUG_FLOW
                usbd_usb_ep_write(usbd, 0, usbd->io);
            }
            //data stage is optional
            else
            {
                usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
#if (USBD_DEBUG_FLOW)
                printf("STATUS OUT\n");
#endif //USBD_DEBUG_FLOW
                usbd_usb_ep_read(usbd, 0, usbd->io, 0);
            }
        }
    }
    else
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT) == BM_REQUEST_RECIPIENT_ENDPOINT)
            usbd_usb_ep_set_stall(usbd, usbd->setup.wIndex);
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
#if (USBD_DEBUG_ERRORS)
        printf("Unhandled ");
        switch (usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT)
        {
        case BM_REQUEST_RECIPIENT_DEVICE:
            printf("DEVICE");
            break;
        case BM_REQUEST_RECIPIENT_INTERFACE:
            printf("INTERFACE");
            break;
        case BM_REQUEST_RECIPIENT_ENDPOINT:
            printf("ENDPOINT");
            break;
        }
        printf(" request\n");

        printf("bmRequestType: %X\n", usbd->setup.bmRequestType);
        printf("bRequest: %X\n", usbd->setup.bRequest);
        printf("wValue: %X\n", usbd->setup.wValue);
        printf("wIndex: %X\n", usbd->setup.wIndex);
        printf("wLength: %X\n", usbd->setup.wLength);
#endif
    }
}

#if (USBD_VSR)
static inline bool usbd_vendor_request(USBD* usbd)
{
    if (usbd->user == INVALID_HANDLE)
        return false;
    IO* io = io_create(usbd->setup.wLength + sizeof(SETUP));
    if (io == NULL)
        return false;

    usbd->setup_state = USB_SETUP_STATE_VENDOR_REQUEST;
    io_push_data(io, &usbd->setup, sizeof(SETUP));
    if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
        io_data_write(io, io_data(usbd->io), usbd->io->data_size);

    io_read(usbd->user, HAL_IO_REQ(HAL_USBD, USBD_VENDOR_REQUEST), 0, io, usbd->setup.wLength);
    return true;
}
#endif //USBD_VSR

static void usbd_setup_process(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT)
    {
    case BM_REQUEST_RECIPIENT_DEVICE:
        if ((usbd->setup.bmRequestType & BM_REQUEST_TYPE) == BM_REQUEST_TYPE_STANDART)
            res = usbd_standart_device_request(usbd);
#if (USBD_VSR)
        else
        {
            if (usbd_vendor_request(usbd))
                return;
        }
#endif
        break;
    case BM_REQUEST_RECIPIENT_INTERFACE:
        res = usbd_interface_request(usbd, usbd->setup.wIndex);
        break;
    case BM_REQUEST_RECIPIENT_ENDPOINT:
        res = usbd_endpoint_request(usbd);
        break;
    }
    usbd_setup_response(usbd, res);
}

static inline void usbd_setup_received(USBD* usbd)
{
    //Back2Back setup received
    if (usbd->setup_state != USB_SETUP_STATE_REQUEST)
    {
#if (USBD_DEBUG_REQUESTS)
        printf("USB B2B SETUP received, state: %d\n", usbd->setup_state);
#endif
        //reset control EP if transaction in progress
        switch (usbd->setup_state)
        {
        case USB_SETUP_STATE_DATA_IN:
        case USB_SETUP_STATE_DATA_IN_ZLP:
        case USB_SETUP_STATE_STATUS_IN:
            usbd_usb_ep_flush(usbd, USB_EP_IN | 0);
            break;
        case USB_SETUP_STATE_DATA_OUT:
        case USB_SETUP_STATE_STATUS_OUT:
            usbd_usb_ep_flush(usbd, 0);
            break;
        default:
            break;
        }

        usbd->setup_state = USB_SETUP_STATE_REQUEST;
    }

    //if data from host - read it first before processing
    if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
    {
        if (usbd->setup.wLength)
        {
            usbd->setup_state = USB_SETUP_STATE_DATA_OUT;
#if (USBD_DEBUG_FLOW)
            printf("DATA OUT: %d\n", usbd->setup.wLength);
#endif //USBD_DEBUG_FLOW
            usbd_usb_ep_read(usbd, 0, usbd->io, usbd->setup.wLength);
        }
        //data stage is optional
        else
            usbd_setup_process(usbd);
    }
    else
        usbd_setup_process(usbd);
}

static inline void usbd_read_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_OUT:
        usbd_setup_process(usbd);
        break;
    case USB_SETUP_STATE_STATUS_OUT:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
#if (USBD_DEBUG_FLOW)
        printf("USBD OK\n");
#endif //USBD_DEBUG_FLOW
        break;
    default:
#if (USBD_DEBUG_ERRORS)
        printf("USBD invalid setup state on read: %d\n", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

static inline void usbd_write_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_IN_ZLP:
        //TX ZLP and switch to normal state
        usbd->setup_state = USB_SETUP_STATE_DATA_IN;
        usbd->io->data_size = 0;
#if (USBD_DEBUG_FLOW)
        printf("DATA IN ZLP\n");
#endif //USBD_DEBUG_FLOW
        usbd_usb_ep_write(usbd, 0, usbd->io);
        break;
    case USB_SETUP_STATE_DATA_IN:
        usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
#if (USBD_DEBUG_FLOW)
        printf("STATUS OUT\n");
#endif //USBD_DEBUG_FLOW
        usbd_usb_ep_read(usbd, 0, usbd->io, 0);
        break;
    case USB_SETUP_STATE_STATUS_IN:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
#if (USBD_DEBUG_FLOW)
        printf("USBD OK\n");
#endif //USBD_DEBUG_FLOW
        break;
    default:
#if (USBD_DEBUG_ERRORS)
        printf("USBD invalid state on write: %d\n", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

static inline void usbd_init(USBD* usbd)
{
    int i;
    usbd->user = INVALID_HANDLE;
    usbd->usb = object_get(SYS_OBJ_USB);
    usbd->io = NULL;
    usbd->suspended = false;
    array_create(&usbd->ifaces, sizeof(USBD_IFACE_ENTRY), 1);
    //at least 5: manufacturer, product, string 0, device, configuration
    array_create(&usbd->descriptors, sizeof(USBD_DESCRIPTOR), 5);

    usbd->ifacecnt = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
        usbd->ep_iface[i] = USBD_INVALID_INTERFACE;
}

static inline void usbd_register_user(USBD* usbd, HANDLE process)
{
    if (usbd->user != INVALID_HANDLE)
        error(ERROR_ALREADY_CONFIGURED);
    else
        usbd->user = process;
}

static inline void usbd_unregister_user(USBD* usbd, HANDLE process)
{
    if (usbd->user != process)
        error(ERROR_ACCESS_DENIED);
    else
        usbd->user = INVALID_HANDLE;
}

#if (USBD_VSR)
static inline void usbd_vendor_response(USBD* usbd, IO* io, int res)
{
    if (usbd->setup_state == USB_SETUP_STATE_VENDOR_REQUEST)
    {
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_DEVICE_TO_HOST)
            io_data_write(usbd->io, io_data(io), res);
        usbd_setup_response(usbd, res);
    }
    io_destroy(io);
}
#endif //USBD_VSR

static inline void usbd_device_request(USBD* usbd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USBD_REGISTER_DESCRIPTOR:
        ipc->param3 = usbd_request_register_descriptor(usbd, (IO*)ipc->param2);
        break;
    case USBD_UNREGISTER_DESCRIPTOR:
        usbd_request_unregister_descriptor(usbd, ipc->param1);
        break;
    case USBD_REGISTER_HANDLER:
        usbd_register_user(usbd, ipc->process);
        break;
    case USBD_UNREGISTER_HANDLER:
        usbd_unregister_user(usbd, ipc->process);
        break;
    case USBD_GET_STATE:
        ipc->param2 = usbd->state;
        break;
#if (USBD_VSR)
    case USBD_VENDOR_REQUEST:
        usbd_vendor_response(usbd, (IO*)ipc->param2, (int)ipc->param3);
        break;
#endif //USBD_VSR
    case IPC_OPEN:
        usbd_open(usbd, ipc->param1);
        break;
    case IPC_CLOSE:
        usbd_close(usbd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void usbd_driver_event(USBD* usbd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_RESET:
        usbd_reset(usbd, ipc->param2);
        //called from ISR, no response
        break;
    case USB_SUSPEND:
        usbd_suspend(usbd);
        //called from ISR, no response
        break;
    case USB_WAKEUP:
        usbd_wakeup(usbd);
        //called from ISR, no response
        break;
    case USB_SETUP:
        ((uint32_t*)(&usbd->setup))[0] = ipc->param2;
        ((uint32_t*)(&usbd->setup))[1] = ipc->param3;
        usbd_setup_received(usbd);
        break;
    case IPC_READ:
        usbd_read_complete(usbd);
        break;
    case IPC_WRITE:
        usbd_write_complete(usbd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void usbd_class_interface_request(USBD* usbd, IPC* ipc, unsigned int iface)
{
    if (iface >= usbd->ifacecnt)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (usbd->suspended)
    {
        error(ERROR_SYNC);
        return;
    }
    IFACE(usbd, iface)->usbd_class->usbd_class_request(usbd, IFACE(usbd, iface)->param, ipc);
}

static inline void usbd_class_endpoint_request(USBD *usbd, IPC* ipc, unsigned int num)
{
    if (num >= USB_EP_COUNT_MAX || usbd->ep_iface[num] >= usbd->ifacecnt)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (usbd->suspended)
    {
        error(ERROR_SYNC);
        return;
    }
    IFACE(usbd, usbd->ep_iface[num])->usbd_class->usbd_class_request(usbd, IFACE(usbd, usbd->ep_iface[num])->param, ipc);
}

void usbd()
{
    USBD usbd;
    IPC ipc;

#if (USBD_DEBUG)
    open_stdout();
#endif

    usbd_init(&usbd);
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_USB:
            //ignore flush/cancel events
            if (((ipc.cmd == HAL_IO_CMD(HAL_USB, IPC_READ)) || (ipc.cmd == HAL_IO_CMD(HAL_USB, IPC_WRITE))) && ((int)ipc.param3 < 0))
                break;

            if ((USB_EP_NUM(USB_NUM(ipc.param1)) == 0) || (USB_NUM(ipc.param1) == USB_HANDLE_DEVICE))
                usbd_driver_event(&usbd, &ipc);
            //decode endpoint interface
            else
                usbd_class_endpoint_request(&usbd, &ipc, USB_EP_NUM(USB_NUM(ipc.param1)));
            break;
        case HAL_USBD:
            usbd_device_request(&usbd, &ipc);
            break;
        case HAL_USBD_IFACE:
            usbd_class_interface_request(&usbd, &ipc, USBD_IFACE_NUM(ipc.param1));
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
        }
        ipc_write(&ipc);
    }
}

#if (USBD_DEBUG)
void usbd_dump(const uint8_t* buf, unsigned int size, const char* header)
{
    int i;
    printf("%s: ", header);
    for (i = 0; i < size; ++i)
        printf("%02X ", buf[i]);
    printf("\n");
}
#endif
