/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIBUSB_H
#define LIBUSB_H

#include "usb.h"

bool libusb_register_descriptor(USBD_DESCRIPTOR_TYPE type, unsigned int index, unsigned int lang, const void *descriptor, unsigned int data_size, unsigned int flags);
bool libusb_register_ascii_string(unsigned int index, unsigned int lang, const char* str);

#endif // LIBUSB_H
