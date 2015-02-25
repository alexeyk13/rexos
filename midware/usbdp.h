/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBDP_H
#define USBDP_H

/*
    USB descriptor parser
 */

#include "../userspace/usb.h"

USB_INTERFACE_DESCRIPTOR_TYPE* usbdp_get_first_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg);
USB_INTERFACE_DESCRIPTOR_TYPE* usbdp_get_next_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start);
USB_DESCRIPTOR_TYPE* usbdp_interface_get_first_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start, unsigned int type);
USB_DESCRIPTOR_TYPE* usbdp_interface_get_next_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_DESCRIPTOR_TYPE* start, unsigned int type);

#endif // USBDP_H
