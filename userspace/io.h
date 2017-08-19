/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IO_H
#define IO_H

#include "types.h"
#include "ipc.h"
#include "kipc.h"

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

/** \addtogroup io io
    interprocess IO

    \{
 */

/**
    \brief get pointer to IO data
    \param io: IO pointer
    \retval data pointer
*/
void* io_data(IO* io);

/**
    \brief get pointer to IO stack
    \param io: IO pointer
    \retval stack pointer
*/
void* io_stack(IO* io);

/**
    \brief push IO stack
    \param io: IO pointer
    \param size: data size
    \retval none
*/
void *io_push(IO* io, unsigned int size);

/**
    \brief push data to IO stack
    \param io: IO pointer
    \param data: data to push
    \param size: data size
    \retval new stack pointer or NULL
*/
void io_push_data(IO* io, void* data, unsigned int size);

/**
    \brief pop IO stack
    \param io: IO pointer
    \param size: data size
    \retval restored stack pointer or NULL
*/
void* io_pop(IO* io, unsigned int size);

/**
    \brief get IO free space
    \param io: IO pointer
    \retval free space
*/
unsigned int io_get_free(IO* io);

/**
    \brief safe write data to IO
    \param io: IO pointer
    \param data: data pointer
    \param size: data size
    \retval data actually written
*/
unsigned int io_data_write(IO* io, const void *data, unsigned int size);


/**
    \brief safe append data to end of IO
    \param io: IO pointer
    \param data: data pointer
    \param size: data size
    \retval data actually written
*/
unsigned int io_data_append(IO* io, const void *data, unsigned int size);


/**
    \brief reset IO to default values
    \param io: IO pointer
    \retval none
*/
void io_reset(IO* io);

/**
    \brief hide part of IO data
    \param io: IO pointer
    \param size: size to hide
    \retval none
*/
void io_hide(IO* io, unsigned int size);

/**
    \brief unhide part of IO data
    \param io: IO pointer
    \param size: size to unhide
    \retval none
*/
void io_unhide(IO* io, unsigned int size);

/**
    \brief restore all hided data
    \param io: IO pointer
    \retval none
*/
void io_show(IO* io);

/**
    \brief creates IO
    \param size: size of io without header
    \retval IO pointer on success.
*/
IO* io_create(unsigned int size);

/**
    \brief send IO write request to another process
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \retval none.
*/
#define io_write(process, cmd, handle, io)                              ipc_post_inline((process), (cmd), (handle), (unsigned int)(io), (io)->data_size)

/**
    \brief send IO write request to exodrivers
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \retval none.
*/
#define io_write_exo(cmd, handle, io)                                   ipc_post_exo((cmd), (handle), (unsigned int)(io), (io)->data_size)

/**
    \brief send IO read request to another process
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \param size: IO data size
    \retval none.
*/
#define io_read(process, cmd, handle, io, size)                         ipc_post_inline((process), (cmd), (handle), (unsigned int)(io), (size))

/**
    \brief send IO read request to exodrivers
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \param size: IO data size
    \retval none.
*/
#define io_read_exo(cmd, handle, io, size)                              ipc_post_exo((cmd), (handle), (unsigned int)(io), (size))

/**
    \brief send IO complete to another process
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \retval none.
*/
#define io_complete(process, cmd, handle, io)                           ipc_post_inline((process), (cmd), (handle), (unsigned int)(io), (io)->data_size)

/**
    \brief send IO complete to another process with error code
    \param cmd: command to send
    \param process: receiver process
    \param handle: user handle
    \param io: pointer to IO structure
    \param param3: ext param or error
    \retval none.
*/
#define io_complete_ex(process, cmd, handle, io, param3)                ipc_post_inline((process), (cmd), (handle), (unsigned int)(io), (param3))

#define io_complete_ex_exo(process, cmd, handle, io, param3)            kipc_post_exo((process), (cmd), (handle), (unsigned int)(io), (unsigned int)(param3))

/**
    \brief wait for async IO completion
    \param process: target process
    \param cmd: command for wait
    \param handle: user handle
    \retval IO size or error.
*/
int io_async_wait(HANDLE process, unsigned int cmd, unsigned int handle);

/**
    \brief wait for async IO completion from exodrivers
    \param cmd: command for wait
    \param handle: user handle
    \retval IO size or error.
*/
#define io_async_wait_exo(cmd, handle)                                  io_async_wait(KERNEL_HANDLE, (cmd), (handle))

/**
    \brief send IO complete to another process. isr version
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \retval none.
*/
#define iio_complete(process, cmd, handle, io)                          ipc_ipost_inline((process), (cmd), (handle), (unsigned int)(io), (io)->data_size)

/**
    \brief send IO complete to another process with error code. isr version
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \param param3: ext param or error
    \retval none.
*/
#define iio_complete_ex(process, cmd, handle, io, param3)               ipc_ipost_inline((process), (cmd), (handle), (unsigned int)(io), (param3))

/**
    \brief send IO write request to another process. Wait for response
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \retval write result.
*/
#define io_write_sync(process, cmd, handle, io)                         get_size((process), (cmd), (handle), (unsigned int)(io), (io)->data_size)

/**
    \brief send IO write request to exodrivers. Wait for response
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \retval write result.
*/
#define io_write_sync_exo(cmd, handle, io)                              get_size_exo((cmd), (handle), (unsigned int)(io), (io)->data_size)

/**
    \brief send IO read request to another process. Wait for response
    \param process: receiver process
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \param size: IO data size
    \retval read result.
*/
#define io_read_sync(process, cmd, handle, io, size)                    get_size((process), (cmd), (handle), (unsigned int)(io), (size))

/**
    \brief send IO read request to exodrivers. Wait for response
    \param cmd: command to send
    \param handle: user handle
    \param io: pointer to IO structure
    \param size: IO data size
    \retval read result.
*/
#define io_read_sync_exo(cmd, handle, io, size)                         get_size_exo((cmd), (handle), (unsigned int)(io), (size))

/**
    \brief destroy IO
    \param io: io to destroy
    \retval IO pointer on success.
*/
void io_destroy(IO* io);

/** \} */ // end of io group

#endif // IO_H
