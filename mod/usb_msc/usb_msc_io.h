/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USB_MSC_IO_H
#define USB_MSC_IO_H

#include "usb.h"

void on_msc_received(EP_CLASS ep, void *param);
void on_msc_sent(EP_CLASS ep, void* param);
void usb_msc_thread(void* param);

void on_storage_buffer_filled(void* param, unsigned int size);
void on_storage_request_buffers(void* param, unsigned int size);

#endif // USB_MSC_IO_H
