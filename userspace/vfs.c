/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "vfs.h"
#include "process.h"
#include <string.h>
#include "stdlib.h"

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
    return io_write_sync(vfs, HAL_IO_REQ(HAL_VFS, VFS_MOUNT), 0, io) == sizeof(VFS_VOLUME_TYPE);
}

void vfs_unmount(HANDLE vfs)
{
    ack(vfs, HAL_REQ(HAL_VFS, VFS_UNMOUNT), 0, 0, 0);
}

bool vfs_record_create(HANDLE vfs, VFS_RECORD_TYPE *vfs_record)
{
    vfs_record->io = io_create(sizeof(VFS_DATA_TYPE));
    if (vfs_record->io == NULL)
        return false;
    vfs_record->current_folder = VFS_ROOT;
    vfs_record->vfs = vfs;
    return true;
}

void vfs_record_destroy(VFS_RECORD_TYPE* vfs_record)
{
    io_destroy(vfs_record->io);
}

HANDLE vfs_find_first(VFS_RECORD_TYPE* vfs_record)
{
    int res;
    *((HANDLE*)io_data(vfs_record->io)) = vfs_record->current_folder;
    vfs_record->io->data_size = sizeof(HANDLE);
    res = io_write_sync(vfs_record->vfs, HAL_IO_REQ(HAL_VFS, VFS_FIND_FIRST), vfs_record->current_folder, vfs_record->io);
    if (res < 0)
        return INVALID_HANDLE;
    return (HANDLE)res;
}

bool vfs_find_next(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle)
{
    int res;
    res = io_read_sync(vfs_record->vfs, HAL_IO_REQ(HAL_VFS, VFS_FIND_NEXT), find_handle, vfs_record->io, sizeof(VFS_DATA_TYPE));
    if (res < 0)
        return false;
    return true;
}

void vfs_find_close(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle)
{
    ack(vfs_record->vfs, HAL_REQ(HAL_VFS, VFS_FIND_CLOSE), find_handle, 0, 0);
}

bool vfs_read_volume_label(VFS_RECORD_TYPE* vfs_record)
{
    int res;
    res = io_read_sync(vfs_record->vfs, HAL_IO_REQ(HAL_VFS, VFS_READ_VOLUME_LABEL), 0, vfs_record->io, sizeof(VFS_DATA_TYPE));
    if (res < 0)
        return false;
    return true;
}
