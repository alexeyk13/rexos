/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "storage.h"
#include "error.h"
#include "process.h"

bool storage_open(HAL hal, HANDLE process, HANDLE user)
{
    get(process, HAL_REQ(hal, IPC_OPEN), user, 0, 0);
    return get_last_error() == ERROR_OK;
}

void storage_get_media_descriptor(HAL hal, HANDLE process, HANDLE user, IO* io)
{
    io_read(process, HAL_IO_REQ(hal, STORAGE_GET_MEDIA_DESCRIPTOR), user, io, sizeof(STORAGE_MEDIA_DESCRIPTOR));
}

STORAGE_MEDIA_DESCRIPTOR* storage_get_media_descriptor_sync(HAL hal, HANDLE process, HANDLE user, IO* io)
{
    if (io_read_sync(process, HAL_IO_REQ(hal, STORAGE_GET_MEDIA_DESCRIPTOR), user, io, sizeof(STORAGE_MEDIA_DESCRIPTOR)) <
            sizeof(STORAGE_MEDIA_DESCRIPTOR))
        return NULL;
    return io_data(io);
}

void storage_read(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int count)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->count = count;
    io_read(process, HAL_IO_REQ(hal, IPC_READ), user, io, count);
}

bool storage_read_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int count)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->count = count;
    return (io_read_sync(process, HAL_IO_REQ(hal, IPC_READ), user, io, count) == count * STORAGE_SECTOR_SIZE);
}

static void storage_write_internal(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int count, unsigned int flags)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->count = count;
    stack->flags = flags;
    io_write(process, HAL_IO_REQ(hal, IPC_WRITE), user, io);
}

static bool storage_write_sync_internal(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int count, unsigned int flags)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->count = count;
    stack->flags = flags;
    return (io_write_sync(process, HAL_IO_REQ(hal, IPC_WRITE), user, io) == count * STORAGE_SECTOR_SIZE);
}

void storage_write(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    storage_write_internal(hal, process, user, io, sector, io->data_size / STORAGE_SECTOR_SIZE, STORAGE_FLAG_WRITE);
}

bool storage_write_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    return storage_write_sync_internal(hal, process, user, io, sector, io->data_size / STORAGE_SECTOR_SIZE, STORAGE_FLAG_WRITE);
}

void storage_erase(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int count)
{
    storage_write_internal(hal, process, user, io, sector, count, 0);
}

bool storage_erase_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int count)
{
    return storage_write_sync_internal(hal, process, user, io, sector, count, 0);
}

void storage_verify(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    storage_write_internal(hal, process, user, io, sector, io->data_size / STORAGE_SECTOR_SIZE, STORAGE_FLAG_VERIFY);
}

bool storage_verify_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    return storage_write_sync_internal(hal, process, user, io, sector, io->data_size / STORAGE_SECTOR_SIZE, STORAGE_FLAG_VERIFY);
}

void storage_write_verify(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    storage_write_internal(hal, process, user, io, sector, io->data_size / STORAGE_SECTOR_SIZE, STORAGE_FLAG_WRITE | STORAGE_FLAG_VERIFY);
}

bool storage_write_verify_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    return storage_write_sync_internal(hal, process, user, io, sector, io->data_size / STORAGE_SECTOR_SIZE, STORAGE_FLAG_WRITE | STORAGE_FLAG_VERIFY);
}

