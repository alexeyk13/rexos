/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_H
#define USBD_H

#include "../../userspace/process.h"
#include "../../userspace/usb.h"
#include "../../userspace/io.h"
#include "../../userspace/types.h"
#include "sys_config.h"

typedef struct _USBD USBD;

typedef struct {
    void (*usbd_class_configured)(USBD*, USB_CONFIGURATION_DESCRIPTOR*);
    void (*usbd_class_reset)(USBD*, void*);
    void (*usbd_class_suspend)(USBD*, void*);
    void (*usbd_class_resume)(USBD*, void*);
    int (*usbd_class_setup)(USBD*, void*, SETUP*, IO*);
    void (*usbd_class_request)(USBD*, void*, IPC*);
} USBD_CLASS;

bool usbd_register_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class, void* param);
bool usbd_unregister_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class);
bool usbd_register_endpoint(USBD* usbd, unsigned int iface, unsigned int ep_num);
bool usbd_unregister_endpoint(USBD* usbd, unsigned int iface, unsigned int ep_num);

//post IPC to user, if configured
void usbd_post_user(USBD* usbd, unsigned int iface, unsigned int num, unsigned int cmd, unsigned int param2, unsigned int param3);
void usbd_io_user(USBD* usbd, unsigned int iface, unsigned int num, unsigned int cmd, IO* io, unsigned int param3);
void usbd_usb_ep_open(USBD* usbd, unsigned int num, USB_EP_TYPE type, unsigned int size);
void usbd_usb_ep_close(USBD* usbd, unsigned int num);
void usbd_usb_ep_flush(USBD* usbd, unsigned int num);
void usbd_usb_ep_set_stall(USBD* usbd, unsigned int num);
void usbd_usb_ep_clear_stall(USBD* usbd, unsigned int num);
void usbd_usb_ep_write(USBD* usbd, unsigned int ep_num, IO* io);
void usbd_usb_ep_read(USBD* usbd, unsigned int ep_num, IO* io, unsigned int size);
int usbd_get_cfg(USBD* usbd, uint8_t iface);
void* usbd_get_cfg_data(USBD* usbd, int i);
int usbd_get_cfg_data_size(USBD* usbd, int i);
#if (USBD_DEBUG)
void usbd_dump(const uint8_t* buf, unsigned int size, const char* header);
#endif

#endif // USBD_H
