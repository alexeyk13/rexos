/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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

#define VFS_VOLUME_HANDLE                                   0xffffffef
#define VFS_BER_HANDLE                                      0xffffffee
#define VFS_FS_HANDLE                                       0xffffffed

#define VFS_ATTR_FOLDER                                     (1 << 0)
#define VFS_ATTR_HIDDEN                                     (1 << 1)
#define VFS_ATTR_READ_ONLY                                  (1 << 2)

#define VFS_MODE_READ                                       (1 << 0)
#define VFS_MODE_WRITE                                      (1 << 1)

#define VFS_CURRENT_FOLDER                                  "."
#define VFS_PARENT_FOLDER                                   ".."
#define VFS_FOLDER_DELIMITER                                '/'

typedef enum {
    VFS_FIND_FIRST = IPC_USER,
    VFS_FIND_NEXT,
    VFS_FIND_CLOSE,
    VFS_CD_UP,
    VFS_CD_PATH,
    VFS_READ_VOLUME_LABEL,
    VFS_REMOVE,
    VFS_MK_FOLDER,
    VFS_GET_FREE,
    VFS_GET_USED,
    VFS_FORMAT,
    VFS_START_TRANSACTION,
    VFS_COMMIT_TRANSACTION,
    VFS_ROLLBACK_TRANSACTION,
    VFS_STAT
} VFS_IPCS;

#define SECTOR_MODE_DIRECT                                  0x00
#define SECTOR_MODE_BER                                     0x01

typedef struct {
    HAL hal;
    unsigned int sector_mode;
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

typedef struct {
    unsigned int block_sectors, fs_blocks;
} VFS_BER_FORMAT_TYPE;

typedef struct {
    unsigned int crc_blocks, bad_blocks, crc_errors_count;
} VFS_BER_STAT_TYPE;

typedef struct {
    unsigned int root_entries;
    unsigned short cluster_sectors, fat_count;
    uint32_t serial;
    char label[9];
} VFS_FAT_FORMAT_TYPE;

HANDLE vfs_create(unsigned int process_size, unsigned int priority);
bool vfs_record_create(HANDLE vfs, VFS_RECORD_TYPE* vfs_record);
void vfs_record_destroy(VFS_RECORD_TYPE* vfs_record);

bool vfs_open_volume(VFS_RECORD_TYPE* vfs_record, VFS_VOLUME_TYPE* volume);
void vfs_close_volume(VFS_RECORD_TYPE* vfs_record);

bool vfs_open_ber(VFS_RECORD_TYPE* vfs_record, unsigned int block_sectors);
void vfs_close_ber(VFS_RECORD_TYPE* vfs_record);
bool vfs_format_ber(VFS_RECORD_TYPE* vfs_record, VFS_BER_FORMAT_TYPE* format);
bool vfs_ber_get_stat(VFS_RECORD_TYPE* vfs_record, VFS_BER_STAT_TYPE* stat);

bool vfs_ber_start_transaction(VFS_RECORD_TYPE* vfs_record);
bool vfs_ber_commit_transaction(VFS_RECORD_TYPE* vfs_record);
bool vfs_ber_rollback_transaction(VFS_RECORD_TYPE* vfs_record);


bool vfs_open_fs(VFS_RECORD_TYPE* vfs_record);
void vfs_close_fs(VFS_RECORD_TYPE* vfs_record);
HANDLE vfs_find_first(VFS_RECORD_TYPE* vfs_record);
bool vfs_find_next(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle);
void vfs_find_close(VFS_RECORD_TYPE* vfs_record, HANDLE find_handle);

void vfs_cd(VFS_RECORD_TYPE* vfs_record, unsigned int folder);
bool vfs_cd_up(VFS_RECORD_TYPE* vfs_record);
bool vfs_cd_path(VFS_RECORD_TYPE* vfs_record, const char* path);
char* vfs_read_volume_label(VFS_RECORD_TYPE* vfs_record);
bool vfs_format(VFS_RECORD_TYPE* vfs_record, VFS_FAT_FORMAT_TYPE* format);

HANDLE vfs_open(VFS_RECORD_TYPE* vfs_record, const char* file_path, unsigned int mode);
bool vfs_seek(VFS_RECORD_TYPE* vfs_record, HANDLE handle, unsigned int pos);
void vfs_read(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io, unsigned int size);
int vfs_read_sync(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io, unsigned int size);
void vfs_write(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io);
int vfs_write_sync(VFS_RECORD_TYPE* vfs_record, HANDLE handle, IO* io);
void vfs_close(VFS_RECORD_TYPE* vfs_record, HANDLE handle);
bool vfs_remove(VFS_RECORD_TYPE* vfs_record, const char* file_path);
bool vfs_mk_folder(VFS_RECORD_TYPE* vfs_record, const char* file_path);
int vfs_get_free(VFS_RECORD_TYPE* vfs_record);
int vfs_get_used(VFS_RECORD_TYPE* vfs_record);

#endif // VFS_H
