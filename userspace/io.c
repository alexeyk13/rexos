/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "io.h"
#include "svc.h"
#include "process.h"
#include "error.h"
#include <string.h>

void* io_data(IO* io)
{
    return (void*)((unsigned int)io + io->data_offset);
}

void* io_stack(IO* io)
{
    return (void*)((unsigned int)io + io->size - io->stack_size);
}

void* io_push(IO* io, unsigned int size)
{
    if (io_get_free(io) < size)
        return NULL;
    io->stack_size += size;
    return io_stack(io);
}

void io_push_data(IO* io, void* data, unsigned int size)
{
    io_push(io, size);
    memcpy(io_stack(io), data, size);
}

void* io_pop(IO* io, unsigned int size)
{
    if (io->stack_size < size)
        return NULL;
    io->stack_size -= size;
    return io_stack(io);
}

unsigned int io_get_free(IO* io)
{
    return io->size - io->data_offset - io->data_size - io->stack_size;
}

unsigned int io_data_write(IO* io, const void* data, unsigned int size)
{
    io->data_size = 0;
    if (io_get_free(io) < size)
        size = io_get_free(io);
    memcpy(io_data(io), data, size);
    io->data_size = size;
    return size;
}

unsigned int io_data_append(IO* io, const void* data, unsigned int size)
{
    if (io_get_free(io) < size)
        size = io_get_free(io);
    memcpy(io_data(io) + io->data_size, data, size);
    io->data_size += size;
    return size;
}

void io_reset(IO* io)
{
    io->data_size = io->stack_size = 0;
    io->data_offset = sizeof(IO);
}

IO* io_create(unsigned int size)
{
    IO* io;
    svc_call(SVC_IO_CREATE, (unsigned int)&io, size, 0);
    if (io)
        io_reset(io);
    return io;
}

int io_write_sync(HANDLE process, unsigned int cmd, unsigned int handle, IO* io)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = io->data_size;
    call(&ipc);
    return ipc.param3;
}

int io_read_sync(HANDLE process, unsigned int cmd, unsigned int handle, IO* io, unsigned int size)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = size;
    call(&ipc);
    return ipc.param3;
}

void io_destroy(IO* io)
{
    if (io != NULL)
        svc_call(SVC_IO_DESTROY, (unsigned int)io, 0, 0);
}
