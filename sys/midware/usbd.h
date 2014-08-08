/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_H
#define USBD_H

#include "../../userspace/process.h"

typedef struct {
    int total_size;                                             //now and follow - header is included
    int qualifier_descriptor_offset;                            //0 if not present
    int configuration_descriptors_offset;
    int other_speed_configuration_descriptors_offset;           //0 if not present
    int string_descriptors_count;
    int string_descriptors_offset;
    //data is following
} USB_DESCRIPTORS_HEADER;

extern const REX __USBD;

#endif // USBD_H
