/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef VFSS_PRIVATE_H
#define VFSS_PRIVATE_H

#include "vfss.h"
#include "ber.h"
#include "fat16.h"
#include "../../userspace/vfs.h"
#include "../../userspace/io.h"
#include "sys_config.h"

typedef struct _VFSS_TYPE {
    IO* io;
    unsigned io_size;
    VFS_VOLUME_TYPE volume;
    unsigned long current_sector, current_size;
#if (VFS_BER)
    BER_TYPE ber;
#endif //VFS_BER
    FAT16_TYPE fat16;
} VFSS_TYPE;


#endif // VFSS_PRIVATE_H
