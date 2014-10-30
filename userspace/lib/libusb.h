/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIBUSB_H
#define LIBUSB_H

#include "../process.h"
#include "../../sys/midware/usbd.h"

bool libusb_register_persistent_descriptor(HANDLE usbd, USBD_DESCRIPTOR_TYPE type, unsigned int index, unsigned int lang, const void *descriptor);


#endif // LIBUSB_H
