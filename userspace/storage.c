/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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

void storage_close(HAL hal, HANDLE process, HANDLE user)
{
    ipc_post_inline(process, HAL_REQ(hal, IPC_CLOSE), user, 0, 0);
}

void storage_get_media_descriptor(HAL hal, HANDLE process, HANDLE user, IO* io)
{
    io_read(process, HAL_IO_REQ(hal, STORAGE_GET_MEDIA_DESCRIPTOR), user, io, sizeof(STORAGE_MEDIA_DESCRIPTOR));
}

void storage_request_notify_state_change(HAL hal, HANDLE process, HANDLE user)
{
    ipc_post_inline(process, HAL_REQ(hal, STORAGE_NOTIFY_STATE_CHANGE), user, 0, 0);
}

void storage_cancel_notify_state_change(HAL hal, HANDLE process, HANDLE user)
{
    ipc_post_inline(process, HAL_CMD(hal, STORAGE_CANCEL_NOTIFY_STATE_CHANGE), user, 0, 0);
}

STORAGE_MEDIA_DESCRIPTOR* storage_get_media_descriptor_sync(HAL hal, HANDLE process, HANDLE user, IO* io)
{
    if (io_read_sync(process, HAL_IO_REQ(hal, STORAGE_GET_MEDIA_DESCRIPTOR), user, io, sizeof(STORAGE_MEDIA_DESCRIPTOR)) <
            sizeof(STORAGE_MEDIA_DESCRIPTOR))
        return NULL;
    return io_data(io);
}

void storage_read(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    io_read(process, HAL_IO_REQ(hal, IPC_READ), user, io, size);
}

bool storage_read_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    return (io_read_sync(process, HAL_IO_REQ(hal, IPC_READ), user, io, size) == size);
}

static void storage_write_internal(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int count, unsigned int flags)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->flags = flags;
    if(flags == STORAGE_FLAG_ERASE_ONLY) io->data_size = count;
    io_write(process, HAL_IO_REQ(hal, IPC_WRITE), user, io);
}

static bool storage_write_sync_internal(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size, unsigned int flags)
{
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = sector;
    stack->flags = flags;
    return (io_read_sync(process, HAL_IO_REQ(hal, IPC_WRITE), user, io, size) == size);
}

void storage_write(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    storage_write_internal(hal, process, user, io, sector, io->data_size, STORAGE_FLAG_WRITE);
}

bool storage_write_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    return storage_write_sync_internal(hal, process, user, io, sector, io->data_size, STORAGE_FLAG_WRITE);
}

void storage_erase(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size)
{
    storage_write_internal(hal, process, user, io, sector, size, 0);
}

bool storage_erase_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size)
{
    return storage_write_sync_internal(hal, process, user, io, sector, size, 0);
}

void storage_verify(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    storage_write_internal(hal, process, user, io, sector, io->data_size, STORAGE_FLAG_VERIFY);
}

bool storage_verify_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    return storage_write_sync_internal(hal, process, user, io, sector, io->data_size, STORAGE_FLAG_VERIFY);
}

void storage_write_verify(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    storage_write_internal(hal, process, user, io, sector, io->data_size, STORAGE_FLAG_WRITE | STORAGE_FLAG_VERIFY);
}

bool storage_write_verify_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector)
{
    return storage_write_sync_internal(hal, process, user, io, sector, io->data_size, STORAGE_FLAG_WRITE | STORAGE_FLAG_VERIFY);
}

void storage_request_activity_notify(HAL hal, HANDLE process, HANDLE user)
{
    ipc_post_inline(process, HAL_REQ(hal, STORAGE_NOTIFY_ACTIVITY), user, 0, 0);
}

void storage_request_eject(HAL hal, HANDLE process, HANDLE user)
{
    ipc_post_inline(process, HAL_CMD(hal, STORAGE_REQUEST_EJECT), user, 0, 0);
}
