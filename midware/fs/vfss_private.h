/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef VFSS_PRIVATE_H
#define VFSS_PRIVATE_H

#include "vfss.h"
#include "ber.h"
#include "../../userspace/vfs.h"
#include "../../userspace/io.h"
#include "sys_config.h"

#if (VFS_SFS)
    #include "sfs.h"
#else
    #include "fat16.h"
#endif  // VFS_SFS


typedef struct _VFSS_TYPE {
    IO* io;
    unsigned io_size;
    VFS_VOLUME_TYPE volume;
    unsigned long current_sector, current_size;
#if (VFS_BER)
    BER_TYPE ber;
#endif //VFS_BER
#if (VFS_SFS)
    SFS_TYPE sfs;
#else
    FAT16_TYPE fat16;
#endif  // VFS_SFS
} VFSS_TYPE;


#endif // VFSS_PRIVATE_H
