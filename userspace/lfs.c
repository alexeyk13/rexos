/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/

#include "lfs.h"
#include "ipc.h"
#include <string.h>

extern void lfss();

HANDLE lfs_create(unsigned int process_size, unsigned int priority)
{
    REX rex;
    rex.name = "Log File System";
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = lfss;
    return process_create(&rex);
}

void lfs_close(HANDLE lfss)
{
    ask(lfss, HAL_REQ(HAL_VFS, IPC_CLOSE), 0, 0, 0);
}

bool lfs_open(HANDLE lfss, LFS_OPEN_TYPE* type)
{
    IO* io;
    uint32_t res;
    io = io_create(sizeof(LFS_OPEN_TYPE));
    memcpy(io_data(io), type, sizeof(LFS_OPEN_TYPE));
    res = get_size(lfss, HAL_IO_REQ(HAL_VFS, IPC_OPEN), 0, (uint32_t)io, 0);
    io_destroy(io);
    return res >=0;
}

bool lfs_format(HANDLE lfss, HANDLE id)
{
    return get_size(lfss, HAL_REQ(HAL_VFS, IPC_FORMAT), id, 0, 0) >=0;
}

int lfs_read(HANDLE lfss, HANDLE id, IO* io, HANDLE record)
{
    return get_size(lfss, HAL_IO_REQ(HAL_VFS, IPC_READ), id, (uint32_t)io, record);
//    io_read_sync(lfss, HAL_IO_REQ(HAL_VFS, IPC_READ), handle, io, size);

}

HANDLE lfs_get_prev(HANDLE lfss, HANDLE id, HANDLE record)
{
    return get_handle(lfss, HAL_REQ(HAL_VFS, IPC_SEEK), id, record, 0);
}

bool lfs_write(HANDLE lfss, HANDLE id, IO* io)
{
    return io_write_sync(lfss, HAL_IO_REQ(HAL_VFS, IPC_WRITE), id, io);
}

void lfs_get_stat(HANDLE lfss, HANDLE id, LFS_STAT* stat)
{
    IO* io;
    io = io_create(sizeof(LFS_STAT));
    ack(lfss, HAL_IO_REQ(HAL_VFS, IPC_GET_STAT), id, (uint32_t)io, 0);
    memcpy(stat, io_data(io), sizeof(LFS_STAT));
    io_destroy(io);
}
