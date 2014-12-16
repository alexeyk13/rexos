/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_H
#define USBD_H

#include "../../userspace/process.h"
#include "../../userspace/usb.h"

typedef struct _USBD USBD;

typedef struct {
    void (*usbd_class_configured)(USBD*, USB_CONFIGURATION_DESCRIPTOR_TYPE*);
    void (*usbd_class_reset)(USBD*, void*);
    void (*usbd_class_suspend)(USBD*, void*);
    void (*usbd_class_resume)(USBD*, void*);
    int (*usbd_class_setup)(USBD*, void*, SETUP*, HANDLE);
    bool (*usbd_class_request)(USBD*, void*, IPC*);
} USBD_CLASS;

extern const REX __USBD;

bool usbd_register_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class, void* param);
bool usbd_unregister_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class);

#endif // USBD_H
