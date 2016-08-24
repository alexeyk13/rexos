/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "vfs.h"
#include "process.h"
#include "io.h"
#include <string.h>

extern void fat16();

HANDLE vfs_create(unsigned int process_size, unsigned int priority)
{
    REX rex;
    rex.name = "FAT16";
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = fat16;
    return process_create(&rex);

}

bool vfs_mount(HANDLE vfs, VFS_VOLUME_TYPE* volume)
{
    IO* io = io_create(sizeof(VFS_VOLUME_TYPE));
    if (io == NULL)
        return false;
    memcpy(io_data(io), volume, sizeof(VFS_VOLUME_TYPE));
    io->data_size = sizeof(VFS_VOLUME_TYPE);
    return io_write_sync(vfs, HAL_IO_REQ(HAL_VFS, IPC_OPEN), 0, io) == sizeof(VFS_VOLUME_TYPE);
}
