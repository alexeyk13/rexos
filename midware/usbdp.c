/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "usbdp.h"

#define D_OFFSET(d, cfg)                            (((unsigned int)(d)) - ((unsigned int)(cfg)))
#define D_NEXT(d, cfg)                              ((USB_DESCRIPTOR_TYPE*)(((unsigned int)(d)) + (d)->bLength))

USB_INTERFACE_DESCRIPTOR_TYPE* usbdp_get_first_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_DESCRIPTOR_TYPE* d;
    for (d = (USB_DESCRIPTOR_TYPE*)cfg; D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_INDEX)
            return (USB_INTERFACE_DESCRIPTOR_TYPE*)d;
    return NULL;
}

USB_INTERFACE_DESCRIPTOR_TYPE* usbdp_get_next_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start)
{
    USB_DESCRIPTOR_TYPE* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == USB_INTERFACE_DESCRIPTOR_INDEX)
            return (USB_INTERFACE_DESCRIPTOR_TYPE*)d;
    return NULL;
}

USB_DESCRIPTOR_TYPE* usbdp_interface_get_first_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start, unsigned int type)
{
    USB_DESCRIPTOR_TYPE* d;
    for (d = D_NEXT(start, cfg); D_OFFSET(d, cfg) < cfg->wTotalLength; d = D_NEXT(d, cfg))
        if (d->bDescriptorType == type || type == 0)
            return d;
    return NULL;
}

USB_DESCRIPTOR_TYPE* usbdp_interface_get_next_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_DESCRIPTOR_TYPE* start, unsigned int type)
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
