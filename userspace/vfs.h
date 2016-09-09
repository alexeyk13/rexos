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

#define VFS_ROOT                                            0

#define VFS_ATTR_FOLDER                                     (1 << 0)
#define VFS_ATTR_HIDDEN                                     (1 << 1)
#define VFS_ATTR_READ_ONLY                                  (1 << 2)

#define VFS_MODE_READ                                       (1 << 0)
#define VFS_MODE_WRITE                                      (1 << 1)

#define VFS_CURRENT_FOLDER                                  "."
#define VFS_PARENT_FOLDER                                   ".."
#define VFS_FOLDER_DELIMITER                                '/'

typedef enum {
    VFS_MOUNT = IPC_USER,
    VFS_UNMOUNT,
    VFS_FIND_FIRST,
    VFS_FIND_NEXT,
    VFS_FIND_CLOSE,
    VFS_CD_UP,
    VFS_CD_PATH,
    VFS_READ_VOLUME_LABEL
} VFS_IPCS;

typedef struct {
    HAL hal;
    HANDLE process, user;
    uint32_t first_sector, sectors_count;
} VFS_VOLUME_TYPE;

typedef struct {
    HANDLE vfs;
    unsigned int current_folder;
    IO* io;
} VFS_RECORD_TYPE;

typedef struct {
    unsigned int item;
    unsigned short attr;
    unsigned int size;
    char name[VFS_MAX_FILE_PATH + 1];
} VFS_FIND_TYPE;

typedef struct {
    unsigned int mode;
    char name[VFS_MAX_FILE_PATH + 1];
} VFS_OPEN_TYPE;

HANDLE vfs_create(unsigned int process_size, unsigned int priority);
bool vfs_mount(HANDLE vfs, VFS_VOLUME_TYPE* volume);
void vfs_unmount(HANDLE vfs);

bool vfs_record_create(HANDLE vfs, VFS_RECORD_TYPE* vfs_record);
void vfs_record_destroy(VFS_RECORD_TYPE* vfs_record);

HANDLE vfs_find_first(VFS_RECORD_TYPE* vfs_record);
bool vfs_find_next(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle);
void vfs_find_close(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle);

void vfs_cd(VFS_RECORD_TYPE* vfs_record, unsigned int folder);
bool vfs_cd_up(VFS_RECORD_TYPE* vfs_record);
bool vfs_cd_path(VFS_RECORD_TYPE* vfs_record, const char* path);
char* vfs_read_volume_label(VFS_RECORD_TYPE* vfs_record);

//Only VFS_MODE_READ supported for now
HANDLE vfs_open(VFS_RECORD_TYPE* vfs_record, const char* file_path, unsigned int mode);
bool vfs_seek(VFS_RECORD_TYPE* vfs_record, HANDLE handle, unsigned int pos);
void vfs_read(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io, unsigned int size);
int vfs_read_sync(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io, unsigned int size);
void vfs_write(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io);
int vfs_write_sync(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io);
void vfs_close(VFS_RECORD_TYPE* vfs_record, HANDLE handle);

#endif // VFS_H
