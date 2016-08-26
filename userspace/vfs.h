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
#include "io.h"
#include "sys_config.h"

#define VFS_ROOT                                            0xffffffff

#define VFS_ATTR_FOLDER                                     (1 << 0)
#define VFS_ATTR_HIDDEN                                     (1 << 1)
#define VFS_ATTR_READ_ONLY                                  (1 << 2)

typedef enum {
    VFS_MOUNT = IPC_USER,
    VFS_UNMOUNT,
    VFS_FIND_FIRST,
    VFS_FIND_NEXT,
    VFS_FIND_CLOSE,
    VFS_READ_VOLUME_LABEL
} VFS_IPCS;

typedef struct {
    HAL hal;
    HANDLE process, user;
    uint32_t first_sector, sectors_count;
} VFS_VOLUME_TYPE;

typedef struct {
    HANDLE vfs;
    HANDLE current_folder;
    IO* io;
} VFS_RECORD_TYPE;

typedef struct {
    HANDLE item_handle;
    unsigned short attr;
    unsigned int size;
    char name[VFS_MAX_FILE_PATH + 1];
} VFS_DATA_TYPE;

HANDLE vfs_create(unsigned int process_size, unsigned int priority);
bool vfs_mount(HANDLE vfs, VFS_VOLUME_TYPE* volume);
void vfs_unmount(HANDLE vfs);
bool vfs_record_create(HANDLE vfs, VFS_RECORD_TYPE* vfs_record);
void vfs_record_destroy(VFS_RECORD_TYPE* vfs_record);
HANDLE vfs_find_first(VFS_RECORD_TYPE* vfs_record);
bool vfs_find_next(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle);
void vfs_find_close(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle);
bool vfs_read_volume_label(VFS_RECORD_TYPE* vfs_record);

#endif // VFS_H
