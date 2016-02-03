/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "storage.h"
#include "error.h"
#include "process.h"

bool storage_open(HAL hal, HANDLE process)
{
    get(process, HAL_REQ(hal, IPC_OPEN), 0, 0, 0);
    return get_last_error() == ERROR_OK;
}

void storage_read(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int count)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->count = count;
    io_read(process, HAL_IO_REQ(hal, IPC_READ), 0, io, count);
}

bool storage_read_sync(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int count)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->count = count;
    return (io_read_sync(process, HAL_IO_REQ(hal, IPC_READ), 0, io, count) == count * STORAGE_SECTOR_SIZE);
}

static void storage_write_internal(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int flags)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->count = io->data_size / STORAGE_SECTOR_SIZE;
    stack->flags = flags;
    io_write(process, HAL_IO_REQ(hal, IPC_WRITE), 0, io);
}

static bool storage_write_sync_internal(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int flags)
{
    unsigned int count;
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    count = stack->count = io->data_size / STORAGE_SECTOR_SIZE;
    stack->flags = flags;
    return (io_write_sync(process, HAL_IO_REQ(hal, IPC_WRITE), 0, io) == count * STORAGE_SECTOR_SIZE);
}

bool storage_write_sync(HAL hal, HANDLE process, IO* io, unsigned int sector)
{
    return storage_write_sync_internal(hal, process, io, sector, STORAGE_FLAG_WRITE);
}

void storage_write(HAL hal, HANDLE process, IO* io, unsigned int sector)
{
    storage_write_internal(hal, process, io, sector, STORAGE_FLAG_WRITE);
}
