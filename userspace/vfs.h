/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef VFS_H
#define VFS_H

#include "types.h"
#include "ipc.h"
#include "storage.h"
#include <stdbool.h>

typedef struct {
    HAL hal;
    HANDLE process, user;
    uint32_t first_sector, sectors_count;
} VFS_VOLUME_TYPE;

HANDLE vfs_create(unsigned int process_size, unsigned int priority);
bool vfs_mount(HANDLE vfs, VFS_VOLUME_TYPE* volume);

#endif // VFS_H
