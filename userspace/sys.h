/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_H
#define SYS_H

#include "stream.h"
#include "object.h"
#include "types.h"
#include "sys_config.h"

/** \addtogroup sys sys
    \{
 */

__STATIC_INLINE bool open_stdout()
{
    HANDLE stream = object_get(SYS_OBJ_STDOUT);
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
    HANDLE stream = object_get(SYS_OBJ_STDIN);
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
