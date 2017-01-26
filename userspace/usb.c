/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "usb.h"
#include "io.h"
#include "error.h"
#include "object.h"
#include "sys_config.h"
#include <string.h>
#include "stdio.h"

#define D_OFFSET(d, cfg)                            (((unsigned int)(d)) - ((unsigned int)(cfg)))
#define D_NEXT(d, cfg)                              ((USB_GENERIC_DESCRIPTOR*)(((unsigned int)(d)) + ((USB_GENERIC_DESCRIPTOR*)(d))->bLength))

extern void usbd();

USB_INTERFACE_DESCRIPTOR* usb_get_first_interface(const USB_CONFIGURATION_DESCRIPTOR* cfg)
{
    USB_GENERIC_DESCRIPTOR* d;
    for (d = (USB_GENERIC_DESCRIPTOR*)cfg; D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
            return (USB_INTERFACE_DESCRIPTOR*)d;
    return NULL;
}

USB_INTERFACE_DESCRIPTOR* usb_get_next_interface(const USB_CONFIGURATION_DESCRIPTOR* cfg, const USB_INTERFACE_DESCRIPTOR* start)
{
    USB_GENERIC_DESCRIPTOR* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
            return (USB_INTERFACE_DESCRIPTOR*)d;
    return NULL;
}

USB_INTERFACE_DESCRIPTOR* usb_find_interface(const USB_CONFIGURATION_DESCRIPTOR* cfg, uint8_t num)
{
    USB_INTERFACE_DESCRIPTOR* iface;
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        if (iface->bInterfaceNumber == num)
            return iface;
    }
    return NULL;
}

void* usb_interface_get_first_descriptor(const USB_CONFIGURATION_DESCRIPTOR* cfg, const USB_INTERFACE_DESCRIPTOR* start, unsigned int type)
{
    USB_GENERIC_DESCRIPTOR* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == type || type == 0)
            return d;
    return NULL;
}

void* usb_interface_get_next_descriptor(const USB_CONFIGURATION_DESCRIPTOR* cfg, const void* start, unsigned int type)
{
    USB_GENERIC_DESCRIPTOR* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
    {
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
            return NULL;
        else if (d->bDescriptorType == type || type == 0)
            return d;
    }
    return NULL;
}

static bool usbd_register_descriptor_internal(HANDLE usbd, IO* io, unsigned int index, unsigned int lang)
{
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs = io_push(io, sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT));

    udrs->index = index;
    udrs->lang = lang;

    io_write_sync(usbd, HAL_IO_REQ(HAL_USBD, USBD_REGISTER_DESCRIPTOR), 0, io);
    io_destroy(io);
    return get_last_error() == ERROR_OK;
}

bool usbd_register_descriptor(HANDLE usbd, const void* d, unsigned int index, unsigned int lang)
{
    unsigned int size;
    const USB_GENERIC_DESCRIPTOR* descriptor = (USB_GENERIC_DESCRIPTOR*)d;
    if (descriptor->bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE || descriptor->bDescriptorType == USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE)
        size = ((const USB_CONFIGURATION_DESCRIPTOR*)d)->wTotalLength;
    else
        size = descriptor->bLength;

    IO* io = io_create(size + sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT) + sizeof(void*));
    if (io == NULL)
        return false;
    *((void**)io_data(io)) = NULL;
    io->data_size = sizeof(void*);

    io_data_append(io, d, size);
    return usbd_register_descriptor_internal(usbd, io, index, lang);
}

bool usbd_register_const_descriptor(HANDLE usbd, const void* d, unsigned int index, unsigned int lang)
{
    IO* io = io_create(sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT) + sizeof(void*));
    if (io == NULL)
        return false;
    *((const void**)io_data(io)) = d;
    io->data_size = sizeof(void*);

    return usbd_register_descriptor_internal(usbd, io, index, lang);
}

bool usbd_register_ascii_string(HANDLE usbd, unsigned int index, unsigned int lang, const char* str)
{
    unsigned int i, len;
    len = strlen(str);
    IO* io = io_create(len * 2 + 2 + sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT) + sizeof(void*));
    if (io == NULL)
        return false;
    *((void**)io_data(io)) = NULL;
    io->data_size = sizeof(void*);

    USB_GENERIC_DESCRIPTOR* descr = (io_data(io) + io->data_size);
    descr->bDescriptorType = USB_STRING_DESCRIPTOR_TYPE;
    descr->bLength = len * 2 + 2;

    for (i = 0; i < len; ++i)
    {
        descr->data[i * 2] = str[i];
        descr->data[i * 2 + 1] = 0x00;
    }
    io->data_size += len * 2 + 2;

    return usbd_register_descriptor_internal(usbd, io, index, lang);
}

HANDLE usbd_create(USB_PORT_TYPE port, unsigned int process_size, unsigned int priority)
{
    char name[19];
    REX rex;
    sprintf(name, "USB_%d device stack", port);
    rex.name = name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = usbd;
    return process_create(&rex);
}

bool usbd_register_configuration(HANDLE usbd, uint16_t cfg, uint16_t iface, const void* d, unsigned int size)
{
    IO* io = io_create(size);
    if (io == NULL)
        return false;
    memcpy(io_data(io), d, size);
    io->data_size = size;

    io_write_sync(usbd, HAL_IO_REQ(HAL_USBD, USBD_REGISTER_CONFIGURATION), (cfg << 16) | iface, io);
    io_destroy(io);
    return get_last_error() == ERROR_OK;
}

bool usbd_unregister_configuration(HANDLE usbd, uint16_t cfg, uint16_t iface)
{
    return get(usbd, HAL_REQ(HAL_USBD, USBD_UNREGISTER_CONFIGURATION), (cfg << 16) | iface, 0, 0) >= 0;
}
