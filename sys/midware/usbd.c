/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usbd.h"
#include "../sys.h"
#include "../usb.h"
#include "../../userspace/lib/stdio.h"
#include "../../userspace/block.h"
#include "../../userspace/direct.h"
#include "../../userspace/lib/stdlib.h"
#include "sys_config.h"
#include <string.h>
#include "../../userspace/lib/array.h"
#include "../file.h"

#define USBD_BLOCK_SIZE                             256

typedef enum {
    USBD_STATE_DEFAULT,
    USBD_STATE_ADDRESSED,
    USBD_STATE_CONFIGURED
} USBD_STATE;

typedef enum {
    USB_SETUP_STATE_REQUEST = 0,
    USB_SETUP_STATE_DATA_IN,
    //in case response is less, than request
    USB_SETUP_STATE_DATA_IN_ZLP,
    USB_SETUP_STATE_DATA_OUT,
    USB_SETUP_STATE_STATUS_IN,
    USB_SETUP_STATE_STATUS_OUT
} USB_SETUP_STATE;


typedef struct {
    HANDLE usb, block;
    //SETUP state machine
    SETUP setup;
    USB_SETUP_STATE setup_state;
    // USBD state machine
    USBD_STATE state;
    bool suspended;
    bool self_powered, remote_wakeup;
    USB_TEST_MODES test_mode;
    //descriptors (LOW SPEED, FULL SPEED, HIGH SPEED, SUPER_SPEED)
    USB_DESCRIPTORS_HEADER* descriptors[4];
    USB_STRING_DESCRIPTORS_HEADER* string_descriptors;
    USB_SPEED speed;
    int ep0_size;
    int configuration, iface, iface_alt;
    ARRAY* classes;
} USBD;

void usbd();

const REX __USBD = {
    //name
    "USB device stack",
    //size
    1024,
    //priority - midware priority
    150,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    usbd
};

static inline void usbd_open(USBD* usbd, HANDLE usb)
{
    if (usbd->usb != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    usbd->block = block_create(USBD_BLOCK_SIZE);
    if ((usbd->block = block_create(USBD_BLOCK_SIZE)) == INVALID_HANDLE)
        return;
    usbd->usb = usb;
    ack(usbd->usb, USB_REGISTER_DEVICE, 0, 0, 0);
}

static inline void usbd_close(USBD* usbd)
{
    if (usbd->usb == INVALID_HANDLE)
        return;

    fflush(usbd->usb, 0);
    fflush(usbd->usb, USB_EP_IN | 0);
    ack(usbd->usb, USB_UNREGISTER_DEVICE, 0, 0, 0);

    block_destroy(usbd->block);

    while (usbd->classes->size)
        array_remove(&usbd->classes, usbd->classes->size - 1);

    usbd->block = usbd->usb = INVALID_HANDLE;
}

static inline void usbd_register_object(ARRAY** ar, HANDLE handle)
{
    int i;
    for (i = 0; i < (*ar)->size; ++i)
        if ((HANDLE)((*ar)->data[i]) == handle)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return;
        }
    if (array_add(ar, 1))
        (*ar)->data[(*ar)->size - 1] = (void*)handle;
}

static inline void usbd_unregister_object(ARRAY** ar, HANDLE handle)
{
    int i;
    for (i = 0; i < (*ar)->size; ++i)
        if ((HANDLE)((*ar)->data[i]) == handle)
        {
            array_remove(ar, i);
            return;
        }
    error(ERROR_NOT_CONFIGURED);
}

void inform(USBD* usbd, unsigned int cmd, unsigned int param1, unsigned int param2)
{
    int i;
    IPC ipc;
    ipc.cmd = USBD_ALERT;
    ipc.param1 = cmd;
    ipc.param2 = param1;
    ipc.param3 = param2;
    for (i = 0; i < usbd->classes->size; ++i)
    {
        ipc.process = (HANDLE)usbd->classes->data[i];
        ipc_post(&ipc);
    }
}

static inline void usbd_set_feature(USBD* usbd, USBD_FEATURES feature, unsigned int param)
{
    switch(feature)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, USB_EP_SET_STALL, param, 0, 0);
        break;
    case USBD_FEATURE_DEVICE_REMOTE_WAKEUP:
        usbd->remote_wakeup = true;
        break;
    case USBD_FEATURE_SELF_POWERED:
        usbd->self_powered = true;
        break;
    default:
        break;
    }
}

static inline void usbd_clear_feature(USBD* usbd, USBD_FEATURES feature, unsigned int param)
{
    switch(feature)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, USB_EP_CLEAR_STALL, param, 0, 0);
        break;
    case USBD_FEATURE_DEVICE_REMOTE_WAKEUP:
        usbd->remote_wakeup = false;
        break;
    case USBD_FEATURE_SELF_POWERED:
        usbd->self_powered = false;
        break;
    default:
        break;
    }
}

static inline USB_DEVICE_DESCRIPTOR_TYPE* get_device_descriptor(USBD* usbd)
{
    return (USB_DEVICE_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + sizeof(USB_DESCRIPTORS_HEADER));
}

static inline USB_CONFIGURATION_DESCRIPTOR_TYPE* get_configuration_descriptor(USBD* usbd, int num)
{
    if (get_device_descriptor(usbd)->bNumConfigurations <= num)
    {
#if (USB_DEBUG_ERRORS)
        printf("USB CONFIGURATION %d descriptor not present\n\r", num);
#endif
        return NULL;
    }
    int i;
    int offset = usbd->descriptors[usbd->speed]->configuration_descriptors_offset;
    for (i = 0; i < num; ++i)
        offset += ((USB_CONFIGURATION_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + offset))->wTotalLength;
    return (USB_CONFIGURATION_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + offset);
}

static inline USB_CONFIGURATION_DESCRIPTOR_TYPE* get_other_speed_configuration_descriptor(USBD* usbd, int num)
{
    if (usbd->descriptors[usbd->speed]->other_speed_configuration_descriptors_offset == 0 || get_device_descriptor(usbd)->bNumConfigurations <= num)
    {
#if (USB_DEBUG_ERRORS)
        printf("USB other speed CONFIGURATION %d descriptor not present\n\r", num);
#endif
        return NULL;
    }
    int i;
    int offset = usbd->descriptors[usbd->speed]->other_speed_configuration_descriptors_offset;
    for (i = 0; i < num; ++i)
        offset += ((USB_CONFIGURATION_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + offset))->wTotalLength;
    return (USB_CONFIGURATION_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + offset);
}

static inline USB_STRING_DESCRIPTOR_TYPE* get_string_descriptor(USBD* usbd, int index, int lang_id)
{
    int i;
    USB_STRING_DESCRIPTOR_OFFSET* offset;
    for (i = 0; i < usbd->string_descriptors->count; ++i)
    {
        offset = (USB_STRING_DESCRIPTOR_OFFSET*)(((char*)usbd->string_descriptors) + sizeof(USB_STRING_DESCRIPTORS_HEADER) + sizeof(USB_STRING_DESCRIPTOR_OFFSET) * i);
        if (offset->index == index && offset->lang_id == lang_id)
        {
            return (USB_STRING_DESCRIPTOR_TYPE*)(((char*)usbd->string_descriptors) + offset->offset);
        }
    }
#if (USB_DEBUG_ERRORS)
    printf("USB: STRING descriptor %d, lang_id: %#X not present\n\r", index, lang_id);
#endif
    return NULL;
}

static inline USB_DEVICE_DESCRIPTOR_TYPE* get_device_qualifier_descriptor(USBD* usbd)
{
    if (usbd->descriptors[usbd->speed]->qualifier_descriptor_offset)
    {
        return (USB_DEVICE_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + usbd->descriptors[usbd->speed]->qualifier_descriptor_offset);
    }
#if (USB_DEBUG_ERRORS)
    printf("USB: QUALIFIER descriptor not present\n\r");
#endif
    return NULL;
}

static inline void usbd_setup_descriptors(USBD* usbd, HANDLE process, USB_SPEED speed, int size)
{
    //must be at least header + device + configuration
    if (size < sizeof(USB_DESCRIPTORS_HEADER) + sizeof(USB_DEVICE_DESCRIPTOR_TYPE))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if ((usbd->descriptors[speed] = realloc(usbd->descriptors[speed], size)) != NULL)
        if (!direct_read(process, usbd->descriptors[speed], size))
        {
            free(usbd->descriptors[speed]);
            usbd->descriptors[speed] = NULL;
        }
}

static inline void usbd_setup_string_descriptors(USBD* usbd, HANDLE process, int size)
{
    if ((usbd->string_descriptors = realloc(usbd->string_descriptors, size)) != NULL)
        if (!direct_read(process, usbd->string_descriptors, size))
        {
            free(usbd->string_descriptors);
            usbd->string_descriptors = NULL;
        }
}

static inline void usbd_reset(USBD* usbd, USB_SPEED speed)
{
    usbd->state = USBD_STATE_DEFAULT;
    usbd->suspended = false;
#if (USB_DEBUG_REQUESTS)
    printf("USB device reset\n\r");
#endif
    usbd->speed = speed;
    usbd->ep0_size = usbd->speed == USB_LOW_SPEED ? 8 : 64;

    USB_EP_OPEN ep_open;
    ep_open.type = USB_EP_CONTROL;
    ep_open.size = usbd->ep0_size;
    fopen_ex(usbd->usb, 0, (void*)&ep_open, sizeof(USB_EP_OPEN));
    fopen_ex(usbd->usb, USB_EP_IN | 0, (void*)&ep_open, sizeof(USB_EP_OPEN));

    inform(usbd, USBD_ALERT_RESET, 0, 0);
}

static inline void usbd_suspend(USBD* usbd)
{
    if (!usbd->suspended)
    {
        usbd->suspended = true;
#if (USB_DEBUG_REQUESTS)
        printf("USB device suspend\n\r");
#endif
        fflush(usbd->usb, 0);
        fflush(usbd->usb, USB_EP_IN | 0);
        if (usbd->state == USBD_STATE_CONFIGURED)
        {
            if (usbd->setup_state != USB_SETUP_STATE_REQUEST)
            {
                fflush(usbd->usb, 0);
                fflush(usbd->usb, USB_EP_IN | 0);
                usbd->setup_state = USB_SETUP_STATE_REQUEST;
            }
            inform(usbd, USBD_ALERT_SUSPEND, 0, 0);
        }
    }
}

static inline void usbd_wakeup(USBD* usbd)
{
    if (usbd->suspended)
    {
        usbd->suspended = false;
#if (USB_DEBUG_REQUESTS)
        printf("USB device wakeup\n\r");
#endif

        if (usbd->state == USBD_STATE_CONFIGURED)
            inform(usbd, USBD_ALERT_WAKEUP, 0, 0);
    }
}

static inline int safecpy_write(USBD* usbd, void* src, int size)
{
    void* ptr = block_open(usbd->block);
    if (size > USBD_BLOCK_SIZE)
        size = USBD_BLOCK_SIZE;
    if (ptr != NULL)
    {
        memcpy(ptr, src, size);
        return size;
    }
    else
        return -1;
}

static inline int send_device_descriptor(USBD* usbd)
{
    USB_DEVICE_DESCRIPTOR_TYPE* device_descriptor = get_device_descriptor(usbd);
#if (USB_DEBUG_REQUESTS)
    printf("USB get DEVICE descriptor\n\r");
#endif
    return safecpy_write(usbd, device_descriptor, device_descriptor->bLength);
}

static inline int send_configuration_descriptor(USBD* usbd, int num)
{
    int res = -1;
    USB_CONFIGURATION_DESCRIPTOR_TYPE* configuration_descriptor;
#if (USB_DEBUG_REQUESTS)
    printf("USB get CONFIGURATION %d descriptor\n\r", num);
#endif
    if ((configuration_descriptor = get_configuration_descriptor(usbd, num)) != NULL)
        res = safecpy_write(usbd, configuration_descriptor, configuration_descriptor->wTotalLength);

    return res;
}

static inline int send_strings_descriptor(USBD* usbd, int index, int lang_id)
{
    int res = -1;
    USB_STRING_DESCRIPTOR_TYPE* string_descriptor;
#if (USB_DEBUG_REQUESTS)
    printf("USB get STRING %d descriptor, LangID: %#X\n\r", index, lang_id);
#endif
    if ((string_descriptor = get_string_descriptor(usbd, index, lang_id)) != NULL)
        res = safecpy_write(usbd, string_descriptor, string_descriptor->bLength);

    return res;
}

static inline int send_device_qualifier_descriptor(USBD* usbd)
{
    int res = -1;
    USB_DEVICE_DESCRIPTOR_TYPE* device_qualifier_descriptor;
#if (USB_DEBUG_REQUESTS)
    printf("USB get DEVICE qualifier descriptor\n\r");
#endif
    if ((device_qualifier_descriptor = get_device_qualifier_descriptor(usbd)) != NULL)
        res =  safecpy_write(usbd, device_qualifier_descriptor, device_qualifier_descriptor->bLength);
    return res;
}

static inline int send_other_speed_configuration_descriptor(USBD* usbd, int num)
{
    int res = -1;
    USB_CONFIGURATION_DESCRIPTOR_TYPE* other_speed_configuration_descriptor;
#if (USB_DEBUG_REQUESTS)
    printf("USB get other speed CONFIGURATION %d descriptor\n\r", num);
#endif
    if ((other_speed_configuration_descriptor = get_other_speed_configuration_descriptor(usbd, num)) != NULL)
        res = safecpy_write(usbd, other_speed_configuration_descriptor, other_speed_configuration_descriptor->wTotalLength);
    return res;
}

static inline int usbd_device_get_status(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get device status\n\r");
#endif
    uint16_t status = 0;
    if (usbd->self_powered)
        status |= 1 << 0;
    if (usbd->remote_wakeup)
        status |= 1 << 1;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

static inline int usbd_device_set_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: device set feature\n\r");
#endif
    //According to documentation the only feature can be set is TEST_MODE
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_TEST_MODE:
        usbd->test_mode = usbd->setup.wIndex >> 16;
        ack(usbd->usb, USB_SET_TEST_MODE, usbd->test_mode, 0, 0);
        res = 0;
        break;
    default:
        break;
    }
    if (res >= 0)
        inform(usbd, USBD_ALERT_FEATURE_SET, usbd->setup.wValue, usbd->setup.wIndex >> 16);
    return res;
}

static inline int usbd_device_clear_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: device clear feature\n\r");
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
    if (res >= 0)
        inform(usbd, USBD_ALERT_FEATURE_SET, usbd->setup.wValue, 0);
    return res;
}

static inline int usbd_set_address(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB set ADDRESS\n\r");
#endif
    ack(usbd->usb, USB_SET_ADDRESS, usbd->setup.wValue, 0, 0);
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
        break;
    }
    return 0;
}

static inline int usbd_get_descriptor(USBD* usbd)
{
    //can be called in any device state
    int res = -1;

    if (usbd->descriptors[usbd->speed] == NULL || usbd->string_descriptors == NULL)
    {
#if (USB_DEBUG_ERRORS)
        printf("USBD: No descriptors set for current speed\n\r");
#endif
        return -1;
    }

    int index = usbd->setup.wValue & 0xff;
    switch (usbd->setup.wValue >> 8)
    {
    case USB_DEVICE_DESCRIPTOR_INDEX:
        res = send_device_descriptor(usbd);
        break;
    case USB_CONFIGURATION_DESCRIPTOR_INDEX:
        res = send_configuration_descriptor(usbd, index);
        break;
    case USB_STRING_DESCRIPTOR_INDEX:
        res = send_strings_descriptor(usbd, index, usbd->setup.wIndex);
        break;
    case USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX:
        res = send_device_qualifier_descriptor(usbd);
        break;
    case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX:
        res = send_other_speed_configuration_descriptor(usbd, index);
        break;
    }

    return res;
}

static inline int usbd_get_configuration(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get configuration\n\r");
#endif
    char configuration = (char)usbd->configuration;
    return safecpy_write(usbd, &configuration, 1);
}

static inline int usbd_set_configuration(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: set configuration %d\n\r", usbd->setup.wValue);
#endif
    int res = -1;
    if (usbd->setup.wValue <= get_device_descriptor(usbd)->bNumConfigurations)
    {
        //read USB 2.0 specification for more details
        if (usbd->state == USBD_STATE_CONFIGURED)
        {
            usbd->configuration = 0;
            usbd->iface = 0;
            usbd->iface_alt = 0;

            usbd->state = USBD_STATE_ADDRESSED;

            inform(usbd, USBD_ALERT_RESET, 0, 0);
        }
        else if (usbd->state == USBD_STATE_ADDRESSED && usbd->setup.wValue)
        {
            usbd->configuration = usbd->setup.wValue;
            usbd->iface = 0;
            usbd->iface_alt = 0;
            usbd->state = USBD_STATE_CONFIGURED;

            inform(usbd, USBD_ALERT_CONFIGURATION_SET, usbd->configuration, 0);
        }
        res = true;
    }
    return res;
}

static inline int usbd_device_request(USBD* usbd)
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

static inline int usbd_interface_get_status(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get interface status\n\r");
#endif
    uint16_t status = 0;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

static inline int usbd_set_interface(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: interface set\n\r");
#endif
    usbd->iface = usbd->setup.wIndex;
    usbd->iface_alt = usbd->setup.wValue;
    inform(usbd, USBD_ALERT_INTERFACE_SET, usbd->iface, usbd->iface_alt);
    return 0;
}

static inline int usbd_get_interface(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: interface get\n\r");
#endif
    uint8_t alt = usbd->iface_alt;
    return safecpy_write(usbd, &alt, 1);
}

static inline int usbd_interface_request(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
        res = usbd_interface_get_status(usbd);
        break;
    case USB_REQUEST_SET_INTERFACE:
        res = usbd_set_interface(usbd);
        break;
    case USB_REQUEST_GET_INTERFACE:
        res = usbd_get_interface(usbd);
        break;
    }
    return res;
}

static inline int usbd_endpoint_get_status(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get endpoint status\n\r");
#endif
    uint16_t status = 0;
    if (get(usbd->usb, USB_EP_IS_STALL, usbd->setup.wIndex & 0xffff, 0, 0))
        status |= 1 << 0;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

static inline int usbd_endpoint_set_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: endpoint set feature\n\r");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, USB_EP_SET_STALL, usbd->setup.wIndex & 0xffff, 0, 0);
        res = 0;
        break;
    default:
        break;
    }
    if (res >= 0)
        inform(usbd, USBD_ALERT_FEATURE_SET, usbd->setup.wValue, usbd->setup.wIndex & 0xffff);
    return res;
}

static inline int usbd_endpoint_clear_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: endpoint clear feature\n\r");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, USB_EP_CLEAR_STALL, usbd->setup.wIndex & 0xffff, 0, 0);
        res = 0;
        break;
    default:
        break;
    }
    if (res >= 0)
        inform(usbd, USBD_ALERT_FEATURE_SET, usbd->setup.wValue, usbd->setup.wIndex & 0xffff);
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

void usbd_setup_process(USBD* usbd)
{
    int i;
    int res = -1;
    IPC ipc, ipcIn;
    switch (usbd->setup.bmRequestType & BM_REQUEST_TYPE)
    {
    case BM_REQUEST_TYPE_STANDART:
        switch (usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT)
        {
        case BM_REQUEST_RECIPIENT_DEVICE:
            res = usbd_device_request(usbd);
            break;
        case BM_REQUEST_RECIPIENT_INTERFACE:
            res = usbd_interface_request(usbd);
            break;
        case BM_REQUEST_RECIPIENT_ENDPOINT:
            res = usbd_endpoint_request(usbd);
            break;
        }
        break;
    case BM_REQUEST_TYPE_CLASS:
    case BM_REQUEST_TYPE_VENDOR:
        ipc.cmd = USB_SETUP;
        ipc.param1 = ((uint32_t*)(&usbd->setup))[0];
        ipc.param2 = ((uint32_t*)(&usbd->setup))[1];
        ipc.param3 = usbd->block;
        for (i = 0; i < usbd->classes->size; ++i)
        {
            ipc.process = (HANDLE)(usbd->classes->data[i]);
            block_send_ipc(usbd->block, ipc.process, &ipc);
            ipc_read_ms(&ipcIn, 0, ipc.process);
            if ((res = ipcIn.param1) >= 0)
                break;
        }
        break;
    }

    if (res > usbd->setup.wLength)
        res = usbd->setup.wLength;
    //success. start transfers
    if (res >= 0)
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
        {
            //data already received, sending status
            usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
            fwrite_null(usbd->usb, USB_EP_IN | 0);
        }
        else
        {
            //response less, than required and multiples of EP0SIZE - we need to send ZLP on end of transfers
            if (res < usbd->setup.wLength && ((res % usbd->ep0_size) == 0))
            {
                if (res)
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN_ZLP;
                    fwrite(usbd->usb, USB_EP_IN | 0, usbd->block, res);
                }
                //if no data at all, but request success, we will send ZLP right now
                else
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                    fwrite_null(usbd->usb, USB_EP_IN | 0);
                }
            }
            else if (res)
            {
                usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                fwrite(usbd->usb, USB_EP_IN | 0, usbd->block, res);
            }
            //data stage is optional
            else
            {
                usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
                fread_null(usbd->usb, 0);
            }
        }
    }
    else
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT) == BM_REQUEST_RECIPIENT_ENDPOINT)
            ack(usbd->usb, USB_EP_SET_STALL, usbd->setup.wIndex, 0, 0);
        else
        {
            ack(usbd->usb, USB_EP_SET_STALL, 0, 0, 0);
            ack(usbd->usb, USB_EP_SET_STALL, USB_EP_IN | 0, 0, 0);
        }
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
#if (USB_DEBUG_ERRORS)
        printf("Unhandled ");
        switch (usbd->setup.bmRequestType & BM_REQUEST_TYPE)
        {
        case BM_REQUEST_TYPE_STANDART:
            printf("STANDART");
            break;
        case BM_REQUEST_TYPE_CLASS:
            printf("CLASS");
            break;
        case BM_REQUEST_TYPE_VENDOR:
            printf("VENDOR");
            break;
        }
        printf(" request\n\r");

        printf("bmRequestType: %X\n\r", usbd->setup.bmRequestType);
        printf("bRequest: %X\n\r", usbd->setup.bRequest);
        printf("wValue: %X\n\r", usbd->setup.wValue);
        printf("wIndex: %X\n\r", usbd->setup.wIndex);
        printf("wLength: %X\n\r", usbd->setup.wLength);
#endif

    }
}

static inline void usbd_setup_received(USBD* usbd)
{
    //Back2Back setup received
    if (usbd->setup_state != USB_SETUP_STATE_REQUEST)
    {
#if (USB_DEBUG_REQUESTS)
        printf("USB B2B SETUP received, state: %d\n\r", usbd->setup_state);
#endif
        //reset control EP if transaction in progress
        switch (usbd->setup_state)
        {
        case USB_SETUP_STATE_DATA_IN:
        case USB_SETUP_STATE_DATA_IN_ZLP:
        case USB_SETUP_STATE_STATUS_IN:
            fflush(usbd->usb, USB_EP_IN | 0);
            break;
        case USB_SETUP_STATE_DATA_OUT:
        case USB_SETUP_STATE_STATUS_OUT:
            fflush(usbd->usb, 0);
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
            fread(usbd->usb, 0, usbd->block, usbd->setup.wLength);
        }
        //data stage is optional
        else
            usbd_setup_process(usbd);
    }
    else
        usbd_setup_process(usbd);
}

void usbd_read_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_OUT:
        usbd_setup_process(usbd);
        break;
    case USB_SETUP_STATE_STATUS_OUT:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    default:
#if (USB_DEBUG_ERRORS)
        printf("USBD invalid setup state on read: %d\n\r", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

void usbd_write_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_IN_ZLP:
        //TX ZLP and switch to normal state
        usbd->setup_state = USB_SETUP_STATE_DATA_IN;
        fwrite_null(usbd->usb, USB_EP_IN | 0);
        break;
    case USB_SETUP_STATE_DATA_IN:
        usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
        fread_null(usbd->usb, 0);
        break;
    case USB_SETUP_STATE_STATUS_IN:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    default:
#if (USB_DEBUG_ERRORS)
        printf("USBD invalid state on write: %s\n\r", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

void usbd()
{
    USBD usbd;
    IPC ipc;

    open_stdout();

    usbd.usb = INVALID_HANDLE;

    usbd.setup_state = USB_SETUP_STATE_REQUEST;

    usbd.state = USBD_STATE_DEFAULT;
    usbd.suspended = false;
    usbd.self_powered = usbd.remote_wakeup = false;
    usbd.test_mode = USB_TEST_MODE_NORMAL;

    usbd.descriptors[USB_LOW_SPEED] = NULL;
    usbd.descriptors[USB_FULL_SPEED] = NULL;
    usbd.descriptors[USB_HIGH_SPEED] = NULL;
    usbd.descriptors[USB_SUPER_SPEED] = NULL;
    usbd.string_descriptors = NULL;
    usbd.speed = USB_LOW_SPEED;
    usbd.ep0_size = 8;
    usbd.configuration = 0;

    array_create(&usbd.classes, 1);

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
//            usbd_info();
            ipc_post(&ipc);
            break;
#endif
        case IPC_OPEN:
            usbd_open(&usbd, (HANDLE)ipc.param1);
            ipc.param1 = 0;
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
            usbd_close(&usbd);
            ipc_post_or_error(&ipc);
            break;
        case USBD_REGISTER_CLASS:
            usbd_register_object(&usbd.classes, (HANDLE)ipc.process);
            ipc_post_or_error(&ipc);
            break;
        case USBD_UNREGISTER_CLASS:
            usbd_unregister_object(&usbd.classes, (HANDLE)ipc.process);
            ipc_post_or_error(&ipc);
            break;
        case USBD_SETUP_DESCRIPTORS:
            usbd_setup_descriptors(&usbd, ipc.process, ipc.param1, ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        case USBD_SETUP_STRING_DESCRIPTORS:
            usbd_setup_string_descriptors(&usbd, ipc.process, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_RESET:
            usbd_reset(&usbd, ipc.param1);
            //called from ISR, no response
            break;
        case USB_SUSPEND:
            usbd_suspend(&usbd);
            //called from ISR, no response
            break;
        case USB_WAKEUP:
            usbd_wakeup(&usbd);
            //called from ISR, no response
            break;
        case IPC_READ_COMPLETE:
            usbd_read_complete(&usbd);
            break;
        case IPC_WRITE_COMPLETE:
            usbd_write_complete(&usbd);
            break;
        case USB_SETUP:
            ((uint32_t*)(&usbd.setup))[0] = ipc.param1;
            ((uint32_t*)(&usbd.setup))[1] = ipc.param2;
            usbd_setup_received(&usbd);
            break;
        case USBD_GET_DRIVER:
            ipc.param1 = usbd.usb;
            ipc_post(&ipc);
            break;
        case USBD_SET_FEATURE:
            usbd_set_feature(&usbd, ipc.param1, ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        case USBD_CLEAR_FEATURE:
            usbd_set_feature(&usbd, ipc.param1, ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}


