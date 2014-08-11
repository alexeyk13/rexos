/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_H
#define USBD_H

#include "../../userspace/process.h"

typedef struct {
    unsigned int total_size;                                             //now and follow - header is included
    unsigned int qualifier_descriptor_offset;                            //0 if not present
    unsigned int configuration_descriptors_offset;
    unsigned int other_speed_configuration_descriptors_offset;           //0 if not present
    //data is following
} USB_DESCRIPTORS_HEADER;

typedef struct {
    unsigned int lang_id;
    unsigned int index;
    unsigned int offset;
} USB_STRING_DESCRIPTOR_OFFSET;

typedef struct {
    unsigned int total_size;
    unsigned int count;
    //offsets follow
    //data follow
} USB_STRING_DESCRIPTORS_HEADER;

extern const REX __USBD;

#endif // USBD_H
