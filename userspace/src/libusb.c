/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "../libusb.h"
#include "../process.h"
#include "../direct.h"
#include "../sys.h"
#include "../ipc.h"
#include "../stdlib.h"
#include <string.h>

bool libusb_register_descriptor(USBD_DESCRIPTOR_TYPE type, unsigned int index, unsigned int lang, const void* d, unsigned int data_size, unsigned int flags)
{
    HANDLE usbd = object_get(SYS_OBJ_USBD);
    uint8_t* buf;
    unsigned int size;
    if (flags & USBD_FLAG_PERSISTENT_DESCRIPTOR)
        size = USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED + sizeof(const void**);
    else
        size = USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED + data_size;
    buf = malloc(size);
    if (buf == NULL)
        return false;
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs = (USBD_DESCRIPTOR_REGISTER_STRUCT*)buf;

    udrs->flags = flags;
    udrs->index = index;
    udrs->lang = lang;

    if (flags & USBD_FLAG_PERSISTENT_DESCRIPTOR)
        *(const void**)(buf + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED) = d;
    else
        memcpy(buf + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED, d, size);

    direct_enable_read(usbd, buf, size);
    ack(usbd, HAL_CMD(HAL_USBD, USBD_REGISTER_DESCRIPTOR), type, size, 0);
    free(buf);
    return get_last_error() == ERROR_OK;
}

bool libusb_register_ascii_string(unsigned int index, unsigned int lang, const char* str)
{
    int i, len;
    void* buf;
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs;
    USB_DESCRIPTOR_TYPE* descr;
    HANDLE usbd = object_get(SYS_OBJ_USBD);
    len = strlen(str);
    //size in utf-16 + header
    buf = malloc(len * 2 + 2 + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED);
    if (buf == NULL)
        return false;
    udrs = (USBD_DESCRIPTOR_REGISTER_STRUCT*)buf;

    udrs->flags = 0;
    udrs->index = index;
    udrs->lang = lang;

    descr = (USB_DESCRIPTOR_TYPE*)(buf + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED);
    descr->bDescriptorType = USB_STRING_DESCRIPTOR_INDEX;
    descr->bLength = len * 2 + 2;

    for (i = 0; i < len; ++i)
    {
        descr->data[i * 2] = str[i];
        descr->data[i * 2 + 1] = 0x00;
    }
    direct_enable_read(usbd, buf, len * 2 + 2 + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED);
    ack(usbd, HAL_CMD(HAL_USBD, USBD_REGISTER_DESCRIPTOR), USB_DESCRIPTOR_STRING, len * 2 + 2 + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED, 0);
    free(buf);
    return get_last_error() == ERROR_OK;
}
