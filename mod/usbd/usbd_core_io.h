/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_CORE_IO_H
#define USBD_CORE_IO_H

void usbd_on_in_writed(EP_CLASS ep, void* param);
void usbd_on_out_readed(EP_CLASS ep, void* param);
void usbd_on_setup(void* param);

#endif // USBD_CORE_IO_H
