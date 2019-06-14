/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/

#ifndef LFS_H
#define LFS_H

#include "sys_config.h"
#include "object.h"
#include "ipc.h"
#include "io.h"
#include <stdint.h>

#define LFS_MAX_FILES         5

typedef enum {
    IPC_FORMAT = IPC_USER,
    IPC_GET_STAT,
} LFS_IPCS;

typedef struct {
    HAL hal;
    unsigned int block_size_bytes;
    unsigned int offset;
    unsigned int files;
    unsigned int sizes[LFS_MAX_FILES];
} LFS_OPEN_TYPE;

typedef struct {
    uint32_t size;
} LFS_STAT;


HANDLE lfs_create(unsigned int process_size, unsigned int priority);
bool lfs_open(HANDLE lfss, LFS_OPEN_TYPE* type);
void lfs_close(HANDLE lfss);
bool lfs_format(HANDLE lfss, HANDLE id);

// HANDLE - pointer to record.
// if return -1 - no record
// if send - 1 - get last record
int lfs_read(HANDLE lfss, HANDLE id, IO* io, HANDLE record);
HANDLE lfs_get_prev(HANDLE lfss, HANDLE id, HANDLE record);
bool lfs_write(HANDLE lfss, HANDLE id, IO* io);
static inline int lfs_read_last(HANDLE lfss, HANDLE id, IO* io)
{
    return lfs_read(lfss, id, io, INVALID_HANDLE);
}

void lfs_get_stat(HANDLE lfss, HANDLE id, LFS_STAT* stat);

#endif // LFS_H
