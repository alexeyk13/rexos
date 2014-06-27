/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ERROR_H
#define ERROR_H

/*
    error.h - error handling
*/

#include "cc_macro.h"
#include "thread.h"

typedef enum {
    ERROR_OK = 0,
    ERROR_IN_PROGRESS,
    ERROR_NOT_SUPPORTED,
    ERROR_TIMEOUT,
    ERROR_INVALID_SVC,
    ERROR_SYNC_OBJECT_DESTROYED,

    ERROR_FILE = 0x20,
    ERROR_FILE_PATH_NOT_FOUND,
    ERROR_FILE_SHARING_VIOLATION,
    ERROR_FILE_ACCESS_DENIED,
    ERROR_FILE_INVALID_NAME,
    ERROR_FILE_PATH_ALREADY_EXISTS,
    ERROR_FILE_PATH_IN_USE,
    ERROR_FOLDER_NOT_EMPTY,
    ERROR_NOT_MOUNTED,
    ERROR_ALREADY_MOUNTED,

    ERROR_MEMORY = 0x30,
    ERROR_OUT_OF_MEMORY,
    ERROR_OUT_OF_SYSTEM_MEMORY,
    ERROR_OUT_OF_PAGED_MEMORY,
    ERROR_POOL_CORRUPTED,
    ERROR_POOL_RANGE_CHECK_FAILED
}ERROR_CODE;

__STATIC_INLINE ERROR_CODE get_last_error()
{
    return __HEAP->error;
}

__STATIC_INLINE void error(ERROR_CODE error)
{
    __HEAP->error = error;
}

__STATIC_INLINE void clear_error()
{
    __HEAP->error = ERROR_OK;
}

#endif // ERROR_H
