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
    HANDLE usb, clas, block;
    //SETUP state machine
    SETUP setup;
    USB_SETUP_STATE setup_state;
    int data_size;
    // USBD state machine
    USBD_STATE state;
    bool suspended;
    //descriptors (LOW SPEED, FULL SPEED, HIGH SPEED, SUPER_SPEED)
    USB_DESCRIPTORS_HEADER* descriptors[4];
    USB_SPEED speed;
    int ep0_size;
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

static inline void usbd_init(USBD* usbd)
{
    ack(usbd->usb, USB_REGISTER_DEVICE, 0, 0, 0);
    ack(usbd->usb, USB_ENABLE, 0, 0, 0);
    usbd->block = block_create(USBD_BLOCK_SIZE);
}

static inline USB_DEVICE_DESCRIPTOR_TYPE* get_device_descriptor(USBD* usbd)
{
    return (USB_DEVICE_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + sizeof(USB_DESCRIPTORS_HEADER));
}

static inline USB_CONFIGURATION_DESCRIPTOR_TYPE* get_configuration_descriptor(USBD* usbd, int num)
{
    int i;
    int offset = usbd->descriptors[usbd->speed]->configuration_descriptors_offset;
    for (i = 0; i < num; ++i)
        offset += ((USB_CONFIGURATION_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + offset))->wTotalLength;
    return (USB_CONFIGURATION_DESCRIPTOR_TYPE*)((char*)(usbd->descriptors[usbd->speed]) + offset);
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

static inline void usbd_reset(USBD* usbd)
{
    usbd->state = USBD_STATE_DEFAULT;
    usbd->suspended = false;
#if (USB_DEBUG_REQUESTS)
    printf("USB device reset\n\r");
#endif
    //TODO: determine size based on speed
    usbd->speed = USB_FULL_SPEED;
    usbd->ep0_size = usbd->speed == USB_LOW_SPEED ? 8 : 64;

    ack(usbd->usb, USB_EP_ENABLE, 0, usbd->ep0_size, USB_EP_CONTROL);
    ack(usbd->usb, USB_EP_ENABLE, USB_EP_IN | 0, usbd->ep0_size, USB_EP_CONTROL);

    //TODO: inform classes & vendors
}

static inline void usbd_suspend(USBD* usbd)
{
    if (!usbd->suspended)
    {
        usbd->suspended = true;
#if (USB_DEBUG_REQUESTS)
        printf("USB device suspend\n\r");
#endif
        ack(usbd->usb, USB_EP_FLUSH, 0, 0, 0);
        ack(usbd->usb, USB_EP_FLUSH, USB_EP_IN | 0, 0, 0);

        //TODO: inform classes & vendors if CONFIGURED
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

        //TODO: inform classes & vendors if CONFIGURED
    }
}

static inline bool safecpy_write(USBD* usbd, void* src, int size)
{
    void* ptr = block_open(usbd->block);
    if (size > USBD_BLOCK_SIZE)
        size = USBD_BLOCK_SIZE;
    if (ptr != NULL)
    {
        memcpy(ptr, src, size);
        usbd->data_size = size;
        return true;
    }
    else
        return false;
}

static inline bool send_device_descriptor(USBD* usbd)
{
    USB_DEVICE_DESCRIPTOR_TYPE* device_descriptor = get_device_descriptor(usbd);
#if (USB_DEBUG_REQUESTS)
    printf("USB get DEVICE descriptor\n\r");
#endif
    return safecpy_write(usbd, device_descriptor, device_descriptor->bLength);
}

static inline bool send_configuration_descriptor(USBD* usbd, int num)
{
    USB_CONFIGURATION_DESCRIPTOR_TYPE* configuration_descriptor;
#if (USB_DEBUG_REQUESTS)
    printf("USB get CONFIGURATION %d descriptor\n\r", num);
#endif
    if (get_device_descriptor(usbd)->bNumConfigurations <= num)
    {
#if (USB_DEBUG_ERRORS)
        printf("USB invalid CONFIGURATION descriptor request - %d\n\r", num);
#endif
        return false;
    }
    configuration_descriptor = get_configuration_descriptor(usbd, num);
    return safecpy_write(usbd, configuration_descriptor, configuration_descriptor->wTotalLength);
}

static inline bool usbd_set_address(USBD* usbd)
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
    return true;
}

static inline bool usbd_get_descriptor(USBD* usbd)
{
    //can be called in any device state
    bool res = false;

    if (usbd->descriptors[usbd->speed] == NULL)
    {
        return false;
#if (USB_DEBUG_ERRORS)
        printf("USBD: No descriptors set for current speed\n\r");
#endif
    }

    int index = usbd->setup.wValue & 0xff;
//    int i;

    switch (usbd->setup.wValue >> 8)
    {
    case USB_DEVICE_DESCRIPTOR_INDEX:
        res = send_device_descriptor(usbd);
        break;
    case USB_CONFIGURATION_DESCRIPTOR_INDEX:
        res = send_configuration_descriptor(usbd, index);
        break;
    case USB_STRING_DESCRIPTOR_INDEX:
        //languages list request
/*		if (index == 0)
        {
            usbd->control.size = usbd->descriptors->p_string_0->bLength;
            memcpy(usbd->control.buf, (char*)usbd->descriptors->p_string_0, usbd->control.size);
            res = true;
        }
        else if (index <= usbd->descriptors->num_of_strings)
        {
            //try to locate language
            for (i = 0; i < usbd->descriptors->num_of_languages; ++i)
                if (usbd->descriptors->p_string_0->data[i] == usbd->control.setup.wIndex)
                {
                    usbd->control.size = usbd->descriptors->ppp_strings[i][index - 1]->bLength;
                    memcpy(usbd->control.buf, (char*)usbd->descriptors->ppp_strings[i][index - 1], usbd->control.size);
                    res = true;
                    break;
                }
        }*/
        break;
    case USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX:
/*		if (usb_get_current_speed(usbd->idx) == USB_HIGH_SPEED && usbd->descriptors->p_usb_qualifier_fs)
        {
            usbd->control.size = usbd->descriptors->p_usb_qualifier_fs->bLength;
            memcpy(usbd->control.buf, (char*)usbd->descriptors->p_usb_qualifier_fs, usbd->control.size);
            res = true;
        }
        else if (usb_get_current_speed(usbd->idx) != USB_HIGH_SPEED && usbd->descriptors->p_usb_qualifier_hs)
        {
            usbd->control.size = usbd->descriptors->p_usb_qualifier_hs->bLength;
            memcpy(usbd->control.buf, (char*)usbd->descriptors->p_usb_qualifier_hs, usbd->control.size);
            res = true;
        }*/
        break;
    case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX:
/*		if (index < usbd->descriptors->num_of_configurations)
        {
            if (usb_get_current_speed(usbd->idx) == USB_HIGH_SPEED)
            {
                if (usbd->descriptors->pp_fs_configurations)
                {
                    usbd->control.size = usbd->descriptors->pp_fs_configurations[index]->wTotalLength;
                    memcpy(usbd->control.buf, (char*)usbd->descriptors->pp_fs_configurations[index], usbd->control.size);
                    res = true;
                }
            }
            else
            {
                usbd->control.size = usbd->descriptors->pp_configurations[index]->wTotalLength;
                memcpy(usbd->control.buf, (char*)usbd->descriptors->pp_configurations[index], usbd->control.size);
                res = true;
            }
        }*/
        break;
    }

    return res;
}

static inline bool usbd_device_request(USBD* usbd)
{
    bool res = false;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
//		res = usbd_device_get_status(usbd);
        break;
    case USB_REQUEST_CLEAR_FEATURE:
        break;
    case USB_REQUEST_SET_FEATURE:
        break;
    case USB_REQUEST_SET_ADDRESS:
        res = usbd_set_address(usbd);
        break;
    case USB_REQUEST_GET_DESCRIPTOR:
        res = usbd_get_descriptor(usbd);
        break;
    //case USB_REQUEST_SET_DESCRIPTOR:
    case USB_REQUEST_GET_CONFIGURATION:
//		res = usbd_get_configuration(usbd);
        break;
    case USB_REQUEST_SET_CONFIGURATION:
//		res = usbd_set_configuration(usbd);
        break;
    }

    return res;
}

static inline void usbd_setup(USBD* usbd)
{
    usbd->data_size = 0;
    bool res = false;

    //Back2Back setup received
    if (usbd->setup_state != USB_SETUP_STATE_REQUEST)
    {
#if (USB_DEBUG_REQUESTS)
        printf("USB B2B SETUP received\n\r");
#endif
        //reset control EP if transaction in progress
        switch (usbd->setup_state)
        {
        case USB_SETUP_STATE_DATA_IN:
        case USB_SETUP_STATE_DATA_IN_ZLP:
        case USB_SETUP_STATE_STATUS_IN:
            ack(usbd->usb, USB_EP_FLUSH, USB_EP_IN | 0, 0, 0);
            break;
        case USB_SETUP_STATE_DATA_OUT:
        case USB_SETUP_STATE_STATUS_OUT:
            ack(usbd->usb, USB_EP_FLUSH, 0, 0, 0);
            break;
        default:
            break;
        }

        usbd->state = USB_SETUP_STATE_REQUEST;
    }

    switch (usbd->setup.bmRequestType & BM_REQUEST_TYPE)
    {
    case BM_REQUEST_TYPE_STANDART:
        switch (usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT)
        {
        case BM_REQUEST_RECIPIENT_DEVICE:
            res = usbd_device_request(usbd);
            break;
        case BM_REQUEST_RECIPIENT_INTERFACE:
//			res = usbd_interface_request(usbd);
            break;
        case BM_REQUEST_RECIPIENT_ENDPOINT:
//			res = usbd_endpoint_request(usbd);
            break;
        }
        break;
    case BM_REQUEST_TYPE_CLASS:
/*		dlist_enum_start((DLIST**)&usbd->class_handlers, &de);
        while (dlist_enum(&de, (DLIST**)&cur))
            if (cur->setup_handler((C_USBD_CONTROL*)&usbd->control, cur->param))
            {
                res = true;
                break;
            }*/
        break;
    case BM_REQUEST_TYPE_VENDOR:
/*		dlist_enum_start((DLIST**)&usbd->vendor_handlers, &de);
        while (dlist_enum(&de, (DLIST**)&cur))
            if (cur->setup_handler((C_USBD_CONTROL*)&usbd->control, cur->param))
            {
                res = true;
                break;
            }*/
        break;
    }

    if (usbd->data_size > usbd->setup.wLength)
        usbd->data_size = usbd->setup.wLength;
    //success. start transfers
    if (res)
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
        {
            if (usbd->data_size)
            {
                usbd->setup_state = USB_SETUP_STATE_DATA_OUT;
                block_send_ipc_inline(usbd->block, usbd->usb, USB_EP_READ, usbd->block, 0, usbd->data_size);
            }
            //data stage is optional
            else
            {
                usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
                ipc_post_inline(usbd->usb, USB_EP_WRITE, INVALID_HANDLE, USB_EP_IN | 0, 0);
            }
        }
        else
        {
            //response less, than required and multiples of EP0SIZE - we need to send ZLP on end of transfers
            if (usbd->data_size < usbd->setup.wLength && (usbd->data_size % usbd->ep0_size) == 0)
            {
                if (usbd->data_size)
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN_ZLP;
                    block_send_ipc_inline(usbd->block, usbd->usb, USB_EP_WRITE, usbd->block, USB_EP_IN | 0, usbd->data_size);
                }
                //if no data at all, but request success, we will send ZLP right now
                else
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                    ipc_post_inline(usbd->usb, USB_EP_WRITE, INVALID_HANDLE, USB_EP_IN | 0, 0);
                }
            }
            else if (usbd->data_size)
            {
                usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                block_send_ipc_inline(usbd->block, usbd->usb, USB_EP_WRITE, usbd->block, USB_EP_IN | 0, usbd->data_size);
            }
            //data stage is optional
            else
            {
                usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
                ipc_post_inline(usbd->usb, USB_EP_READ, INVALID_HANDLE, 0, 0);
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
#if (USB_DEBUG_ERRORS)
        printf("Unhandled USB SETUP:\n\r");
        printf("bmRequestType: %X\n\r", usbd->setup.bmRequestType);
        printf("bRequest: %X\n\r", usbd->setup.bRequest);
        printf("wValue: %X\n\r", usbd->setup.wValue);
        printf("wIndex: %X\n\r", usbd->setup.wIndex);
        printf("wLength: %X\n\r", usbd->setup.wLength);
#endif

    }
}

void usbd_read_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_OUT:
        usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
        ipc_post_inline(usbd->usb, USB_EP_WRITE, INVALID_HANDLE, USB_EP_IN | 0, 0);
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
        ipc_post_inline(usbd->usb, USB_EP_WRITE, INVALID_HANDLE, USB_EP_IN | 0, 0);
        break;
    case USB_SETUP_STATE_DATA_IN:
        usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
        ipc_post_inline(usbd->usb, USB_EP_READ, INVALID_HANDLE, 0, 0);
        break;
    case USB_SETUP_STATE_STATUS_IN:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    default:
#if (USB_DEBUG_ERRORS)
        printf("USBD invalid state on write: %s\n\r", usbd->setup_state);
#endif
        usbd->state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

void usbd()
{
    USBD usbd;
    IPC ipc;

    open_stdout();

    usbd.usb = usbd.clas = INVALID_HANDLE;

    usbd.block = INVALID_HANDLE;
    usbd.setup_state = USB_SETUP_STATE_REQUEST;
    usbd.data_size = 0;

    usbd.state = USBD_STATE_DEFAULT;
    usbd.suspended = false;

    usbd.descriptors[USB_LOW_SPEED] = NULL;
    usbd.descriptors[USB_FULL_SPEED] = NULL;
    usbd.descriptors[USB_HIGH_SPEED] = NULL;
    usbd.descriptors[USB_SUPER_SPEED] = NULL;
    usbd.speed = USB_LOW_SPEED;
    usbd.ep0_size = 8;

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
        case SYS_GET_INFO:
//            cdc_info();
            ipc_post(&ipc);
            break;
#endif
        case USB_SETUP_DRIVER:
            usbd.usb = ipc.param1;
            usbd_init(&usbd);
            ipc_post_or_error(&ipc);
            break;
        case USB_SETUP_DESCRIPTORS:
            usbd_setup_descriptors(&usbd, ipc.process, ipc.param1, ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        case USB_RESET:
            usbd_reset(&usbd);
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
        case USB_EP_READ_COMPLETE:
            usbd_read_complete(&usbd);
            break;
        case USB_EP_WRITE_COMPLETE:
            usbd_write_complete(&usbd);
            break;
        case USB_SETUP:
            ((uint32_t*)(&usbd.setup))[0] = ipc.param1;
            ((uint32_t*)(&usbd.setup))[1] = ipc.param2;
            usbd_setup(&usbd);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}


