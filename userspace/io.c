/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "io.h"
#include "svc.h"
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

void io_push(IO* io, unsigned int size)
{
    io->stack_size += size;
}

void io_push_data(IO* io, void* data, unsigned int size)
{
    io->stack_size += size;
    memcpy(io_stack(io), data, size);
}

void io_pop(IO* io, unsigned int size)
{
    io->stack_size -= size;
}

unsigned int io_get_free(IO* io)
{
    return io->size - io->data_offset - io->data_size - io->stack_size;
}

unsigned int io_data_write(IO* io, const void* data, unsigned int size)
{
    if (io_get_free(io) < size)
        size = io_get_free(io);
    memcpy(io_data(io), data, size);
    io->data_size = size;
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

void io_send(IPC* ipc)
{
    svc_call(SVC_IO_SEND, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, 0);
}

void io_write(unsigned int cmd, HANDLE process, unsigned int handle, IO* io)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = io->data_size;
    svc_call(SVC_IO_SEND, (unsigned int)(io->kio), (unsigned int)&ipc, 0);
}

void io_read(unsigned int cmd, HANDLE process, unsigned int handle, IO* io, unsigned int size)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = size;
    svc_call(SVC_IO_SEND, (unsigned int)(io->kio), (unsigned int)&ipc, 0);
}

void io_complete(unsigned int cmd, HANDLE process, unsigned int handle, IO* io)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = io->data_size;
    svc_call(SVC_IO_SEND, (unsigned int)(io->kio), (unsigned int)&ipc, 0);
}

void io_complete_error(unsigned int cmd, HANDLE process, unsigned int handle, IO* io, int error)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = error;
    svc_call(SVC_IO_SEND, (unsigned int)(io->kio), (unsigned int)&ipc, 0);
}

void io_send_error(IPC* ipc, unsigned int error)
{
    ipc->param3 = error;
    svc_call(SVC_IO_SEND, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, 0);
}

void iio_send(IPC* ipc)
{
    __GLOBAL->svc_irq(SVC_IO_SEND, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, 0);
}

void iio_complete(unsigned int cmd, HANDLE process, unsigned int handle, IO* io)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = io->data_size;
    __GLOBAL->svc_irq(SVC_IO_SEND, (unsigned int)(io->kio), (unsigned int)&ipc, 0);
}

void iio_complete_error(unsigned int cmd, HANDLE process, unsigned int handle, IO* io, int error)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = error;
    __GLOBAL->svc_irq(SVC_IO_SEND, (unsigned int)(io->kio), (unsigned int)&ipc, 0);
}

void io_call_ipc(IPC* ipc)
{
    TIME time;
    time.sec = time.usec = 0;
    svc_call(SVC_IO_CALL, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, (unsigned int)&time);
}

int io_call(HANDLE process, unsigned int cmd, unsigned int handle, IO* io, int size)
{
    IPC ipc;
    TIME time;
    ipc.process = process;
    ipc.cmd = cmd;
    ipc.param1 = handle;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = (unsigned int)size;
    time.sec = time.usec = 0;
    svc_call(SVC_IO_CALL, (unsigned int)(io->kio), (unsigned int)&ipc, (unsigned int)&time);
    return ipc.param3;
}

void io_destroy(IO* io)
{
    svc_call(SVC_IO_DESTROY, (unsigned int)(io->kio), 0, 0);
}
