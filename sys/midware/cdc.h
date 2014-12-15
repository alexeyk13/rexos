/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CDC_H
#define CDC_H

#include "../../userspace/process.h"
#include "../../userspace/usb.h"


//TODO: pass interfaces here
void* usbd_class_configured(USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg);
void usbd_class_reset(void* param);
void usbd_class_suspend(void* param);
void usbd_class_resume(void* param);

int usbd_class_setup(void* param, SETUP* setup, HANDLE block);
bool usbd_class_request(void* param, IPC* ipc);

#endif // CDC_H
