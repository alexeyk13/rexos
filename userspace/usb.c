/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "usb.h"
#include "io.h"
#include "error.h"
#include "object.h"
#include <string.h>

bool usbd_register_descriptor(const void* d, unsigned int index, unsigned int lang)
{
    unsigned int size;
    const USB_DESCRIPTOR_TYPE* descriptor = (USB_DESCRIPTOR_TYPE*)d;
    if (descriptor->bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_INDEX || descriptor->bDescriptorType == USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX)
        size = ((const USB_CONFIGURATION_DESCRIPTOR_TYPE*)d)->wTotalLength;
    else
        size = descriptor->bLength;

    IO* io = io_create(size + sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT));
    if (io == NULL)
        return false;
    io_push(io, sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT));
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs = io_stack(io);

    udrs->index = index;
    udrs->lang = lang;

    io_data_write(io, d, size);
    io_call(object_get(SYS_OBJ_USBD), HAL_CMD(HAL_USBD, USBD_REGISTER_DESCRIPTOR), 0, io, io->data_size);
    io_destroy(io);
    return get_last_error() == ERROR_OK;
}

bool usbd_register_ascii_string(unsigned int index, unsigned int lang, const char* str)
{
    unsigned int i, len;
    len = strlen(str);
    IO* io = io_create(len * 2 + 2 + sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT));
    if (io == NULL)
        return false;
    io_push(io, sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT));
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs = io_stack(io);

    udrs->index = index;
    udrs->lang = lang;

    USB_DESCRIPTOR_TYPE* descr = io_data(io);
    descr->bDescriptorType = USB_STRING_DESCRIPTOR_INDEX;
    descr->bLength = len * 2 + 2;

    for (i = 0; i < len; ++i)
    {
        descr->data[i * 2] = str[i];
        descr->data[i * 2 + 1] = 0x00;
    }
    io->data_size = len * 2 + 2;
    io_call(object_get(SYS_OBJ_USBD), HAL_CMD(HAL_USBD, USBD_REGISTER_DESCRIPTOR), 0, io, io->data_size);
    io_destroy(io);
    return get_last_error() == ERROR_OK;
}

