/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_usb.h"
#include "../userspace/direct.h"
#include "../userspace/object.h"
#include "../userspace/usb.h"
#include "sys_config.h"

bool lib_usb_register_persistent_descriptor(USBD_DESCRIPTOR_TYPE type, unsigned int index, unsigned int lang, const void* descriptor)
{
    HANDLE usbd = object_get(SYS_OBJ_USBD);
    char buf[USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED + sizeof(const void**)];
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs = (USBD_DESCRIPTOR_REGISTER_STRUCT*)buf;

    udrs->flags = USBD_FLAG_PERSISTENT_DESCRIPTOR;
    udrs->index = index;
    udrs->lang = lang;
    *(const void**)(buf + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED) = descriptor;
    direct_enable_read(usbd, buf, USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED + sizeof(const void**));
    ack(usbd, USBD_REGISTER_DESCRIPTOR, type, USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED + sizeof(const void**), 0);
    return get_last_error() == ERROR_OK;
}

const LIB_USB __LIB_USB = {
    lib_usb_register_persistent_descriptor
};
