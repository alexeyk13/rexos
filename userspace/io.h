/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IO_H
#define IO_H

#include "types.h"
#include "cc_macro.h"
#include "svc.h"
#include "error.h"
#include "ipc.h"

#pragma pack(push, 1)

/*
 *      IO structure:
 *
 *      +-------------------------+
 *      |      IO SYS header      |
 *      +-------------------------+
 *      | adjustable user header  |
 *      +-------------------------+
 *      |                         |
 *      |                         |
 *      |        user data        |
 *      |                         |
 *      |                         |
 *      +-------------------------+
 *      |    user params stack    |
 *      +-------------------------+
 */

typedef struct {
    HANDLE kio;
    unsigned int size, data_offset, data_size, stack_size;
} IO;

#pragma pack(pop)

#define IO_DATA(io)                                     ((void*)(((unsigned int)(io)) + (io)->data_offset))
#define IO_STACK(io)                                    ((void*)(((unsigned int)(io)) + (io)->size - (io)->stack_size))
#define IO_PUSH(io, size)                               ((io)->stack_size += (size))
#define IO_POP(io, size)                                ((io)->stack_size -= (size))
#define IO_FREE(io)                                     ((io)->size - (io)->data_offset - (io)->data_size - (io)->stack_size)


/** \addtogroup io io
    interprocess IO

    \{
 */

/**
    \brief reset IO to default values
    \param io: IO pointer
    \retval none
*/
__STATIC_INLINE void io_reset(IO* io)
{
    io->data_size = io->stack_size = 0;
    io->data_offset = sizeof(IO);
}

/**
    \brief creates IO
    \param size: size of io without header
    \retval IO pointer on success.
*/
__STATIC_INLINE IO* io_create(unsigned int size)
{
    IO* io;
    svc_call(SVC_IO_CREATE, (unsigned int)&io, size, 0);
    if (io)
        io_reset(io);
    return io;
}

/**
    \brief send IO to another process
    \param ipc: IPC with IO as param
    \retval none.
*/
__STATIC_INLINE void io_send(IPC* ipc)
{
    svc_call(SVC_IO_SEND, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, 0);
}

/**
    \brief send IO to another process with error set
    \param ipc: IPC with IO as param
    \param error: error code
    \retval none.
*/
__STATIC_INLINE void io_send_error(IPC* ipc, unsigned int error)
{
    ipc->param3 = error;
    svc_call(SVC_IO_SEND, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, 0);
}

/**
    \brief send IO to another process. isr version
    \param ipc: IPC with IO as param
    \retval none.
*/
__STATIC_INLINE void iio_send(IPC* ipc)
{
    __GLOBAL->svc_irq(SVC_IO_SEND, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, 0);
}

/**
    \brief send IO to another process. Wait for response.
    \param ipc: IPC with IO as param
    \retval none.
*/
__STATIC_INLINE void io_call_ipc(IPC* ipc)
{
    TIME time;
    time.sec = time.usec = 0;
    svc_call(SVC_IO_CALL, (unsigned int)(((IO*)(ipc->param2))->kio), (unsigned int)ipc, (unsigned int)&time);
}

/**
    \brief send IO to another process. Wait for response.
    \param process: target process
    \param cmd: cmd to call
    \param handle: user handle
    \param size: size of transfer
    \retval none.
*/
__STATIC_INLINE int io_call(HANDLE process, unsigned int cmd, unsigned int handle, IO* io, int size)
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

/**
    \brief destroy IO
    \param io: io to destroy
    \retval IO pointer on success.
*/
__STATIC_INLINE void io_destroy(IO* io)
{
    svc_call(SVC_IO_DESTROY, (unsigned int)(io->kio), 0, 0);
}

/** \} */ // end of io group

#endif // IO_H
