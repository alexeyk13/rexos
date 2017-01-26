/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef VFSS_H
#define VFSS_H

#include <stdbool.h>

typedef struct _VFSS_TYPE VFSS_TYPE;

void* vfss_get_buf(VFSS_TYPE* vfss);
unsigned int vfss_get_buf_size(VFSS_TYPE* vfss);
void vfss_resize_buf(VFSS_TYPE* vfss, unsigned int size);
unsigned int vfss_get_volume_offset(VFSS_TYPE* vfss);
unsigned int vfss_get_volume_sectors(VFSS_TYPE* vfss);

void* vfss_read_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size);
bool vfss_write_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size);
bool vfss_zero_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned count);

#endif // VFSS_H
