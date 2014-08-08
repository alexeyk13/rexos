/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_H
#define SYS_H

#include "../userspace/ipc.h"
#include "../userspace/stream.h"
#include "../userspace/lib/types.h"

typedef enum {
    SYS_SET_STDIO = IPC_SYSTEM + 1,                             //!< Will be called for objects, created before global stdout STREAM is set
    SYS_GET_OBJECT,                                             //!< Get system object. Temporaily solution before FS is ready
    SYS_SET_OBJECT,                                             //!< Set system object. Temporaily solution before FS is ready
    SYS_GET_INFO,
}SYS_IPCS;

typedef enum {
    SYS_OBJECT_CORE,
    SYS_OBJECT_UART,
    SYS_OBJECT_STDOUT_STREAM,
    SYS_OBJECT_STDIN_STREAM
}SYS_OBJECT;

/** \addtogroup sys sys
    \{
 */


/**
    \brief call to system.
    \param ipc: \ref ipc to post/read.
    \retval true on success
*/

__STATIC_INLINE bool sys_call(IPC* ipc)
{
    ipc->process = __HEAP->system;
    return call(ipc);
}

/**
    \brief call to sys with no return value
    \param cmd: command to post
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval true on success
*/

__STATIC_INLINE bool sys_ack(unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    return ack(__HEAP->system, cmd, param1, param2, param3);
}

/**
    \brief get value from sys
    \param cmd: command to post
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval param1
*/

__STATIC_INLINE unsigned int sys_get(unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    return get(__HEAP->system, cmd, param1, param2, param3);
}


/**
    \brief get sys object handle. Temporaily, before FS will be ready
    \param object: sys object type
    \retval handle on success, INVALID_HANDLE on error
*/

__STATIC_INLINE HANDLE sys_get_object(int object)
{
    return sys_get(SYS_GET_OBJECT, object, 0, 0);
}

/**
    \brief open stdout
    \retval true on success
*/

__STATIC_INLINE bool open_stdout()
{
    HANDLE stream = sys_get_object(SYS_OBJECT_STDOUT_STREAM);
    if (stream != INVALID_HANDLE)
        __HEAP->stdout = stream_open(stream);
    return __HEAP->stdout != INVALID_HANDLE;
}

/**
    \brief open stdid
    \retval true on success
*/

__STATIC_INLINE bool open_stdin()
{
    HANDLE stream = sys_get_object(SYS_OBJECT_STDIN_STREAM);
    if (stream != INVALID_HANDLE)
        __HEAP->stdin = stream_open(stream);
    return __HEAP->stdin != INVALID_HANDLE;
}

/**
    \brief close stdout
    \retval none
*/

__STATIC_INLINE void close_stdout()
{
    if (__HEAP->stdout != INVALID_HANDLE)
        stream_close(__HEAP->stdout);
}

/**
    \brief close stdid
    \retval none
*/

__STATIC_INLINE void close_stdin()
{
    if (__HEAP->stdin != INVALID_HANDLE)
        stream_close(__HEAP->stdin);
}

/** \} */ // end of sys group

#endif // SYS_H
