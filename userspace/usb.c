/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "usb.h"
#include "io.h"
#include "error.h"
#include "object.h"
#include "sys_config.h"
#include <string.h>

#define D_OFFSET(d, cfg)                            (((unsigned int)(d)) - ((unsigned int)(cfg)))
#define D_NEXT(d, cfg)                              ((USB_DESCRIPTOR_TYPE*)(((unsigned int)(d)) + (d)->bLength))

USB_INTERFACE_DESCRIPTOR_TYPE* usb_get_first_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_DESCRIPTOR_TYPE* d;
    for (d = (USB_DESCRIPTOR_TYPE*)cfg; D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_INDEX)
            return (USB_INTERFACE_DESCRIPTOR_TYPE*)d;
    return NULL;
}

USB_INTERFACE_DESCRIPTOR_TYPE* usb_get_next_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start)
{
    USB_DESCRIPTOR_TYPE* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_INDEX)
            return (USB_INTERFACE_DESCRIPTOR_TYPE*)d;
    return NULL;
}

USB_DESCRIPTOR_TYPE* usb_interface_get_first_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start, unsigned int type)
{
    USB_DESCRIPTOR_TYPE* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == type || type == 0)
            return d;
    return NULL;
}

USB_DESCRIPTOR_TYPE* usb_interface_get_next_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_DESCRIPTOR_TYPE* start, unsigned int type)
{
    USB_DESCRIPTOR_TYPE* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
    {
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_INDEX)
            return NULL;
        else if (d->bDescriptorType == type || type == 0)
            return d;
    }
    return NULL;
}

static bool usbd_register_descriptor_internal(IO* io, unsigned int index, unsigned int lang)
{
    io_push(io, sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT));
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs = io_stack(io);

    udrs->index = index;
    udrs->lang = lang;

    io_write_sync(object_get(SYS_OBJ_USBD), HAL_CMD(HAL_USBD, USBD_REGISTER_DESCRIPTOR), 0, io);
    io_destroy(io);
    return get_last_error() == ERROR_OK;
}

bool usbd_register_descriptor(const void* d, unsigned int index, unsigned int lang)
{
    unsigned int size;
    const USB_DESCRIPTOR_TYPE* descriptor = (USB_DESCRIPTOR_TYPE*)d;
    if (descriptor->bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_INDEX || descriptor->bDescriptorType == USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX)
        size = ((const USB_CONFIGURATION_DESCRIPTOR_TYPE*)d)->wTotalLength;
    else
        size = descriptor->bLength;

    IO* io = io_create(size + sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT) + sizeof(void*));
    if (io == NULL)
        return false;
    *((void**)io_data(io)) = NULL;
    io->data_size = sizeof(void*);

    io_data_append(io, d, size);
    return usbd_register_descriptor_internal(io, index, lang);
}

bool usbd_register_const_descriptor(const void* d, unsigned int index, unsigned int lang)
{
    IO* io = io_create(sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT) + sizeof(void*));
    if (io == NULL)
        return false;
    *((const void**)io_data(io)) = d;
    io->data_size = sizeof(void*);

    return usbd_register_descriptor_internal(io, index, lang);
}

bool usbd_register_ascii_string(unsigned int index, unsigned int lang, const char* str)
{
    unsigned int i, len;
    len = strlen(str);
    IO* io = io_create(len * 2 + 2 + sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT) + sizeof(void*));
    if (io == NULL)
        return false;
    *((void**)io_data(io)) = NULL;
    io->data_size = sizeof(void*);

    USB_DESCRIPTOR_TYPE* descr = (io_data(io) + io->data_size);
    descr->bDescriptorType = USB_STRING_DESCRIPTOR_INDEX;
    descr->bLength = len * 2 + 2;

    for (i = 0; i < len; ++i)
    {
        descr->data[i * 2] = str[i];
        descr->data[i * 2 + 1] = 0x00;
    }
    io->data_size += len * 2 + 2;

    return usbd_register_descriptor_internal(io, index, lang);
}

