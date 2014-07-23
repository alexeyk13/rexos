/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DIRECT_H
#define DIRECT_H

#include "lib/types.h"
#include "cc_macro.h"
#include "svc.h"
#include "error.h"

/** \addtogroup direct direct
    interprocess direct IO

    \{
 */


/**
    \brief enable direct read. Grant other process to direct read
    \param process: handle of process, enabled directed IO
    \param addr: address
    \param size: max size
    \retval none
*/
__STATIC_INLINE void direct_enable_read(HANDLE process, void* addr, unsigned int size)
{
    __HEAP->direct_mode |= DIRECT_READ;
    __HEAP->direct_process = process;
    __HEAP->direct_addr = addr;
    __HEAP->direct_size = size;
}

/**
    \brief enable direct write. Grant other process to direct write
    \param process: handle of process, enabled directed IO
    \param addr: address
    \param size: max size
    \retval none
*/
__STATIC_INLINE void direct_enable_write(HANDLE process, void* addr, unsigned int size)
{
    __HEAP->direct_mode |= DIRECT_WRITE;
    __HEAP->direct_process = process;
    __HEAP->direct_addr = addr;
    __HEAP->direct_size = size;
}

/**
    \brief enable direct read and write. Grant other process to direct read and write
    \param process: handle of process, enabled directed IO
    \param addr: address
    \param size: max size
    \retval none
*/
__STATIC_INLINE void direct_enable_read_write(HANDLE process, void* addr, unsigned int size)
{
    __HEAP->direct_mode |= DIRECT_READ_WRITE;
    __HEAP->direct_process = process;
    __HEAP->direct_addr = addr;
    __HEAP->direct_size = size;
}

/**
    \brief do direct read
    \param process: handle of process, enabled directed IO
    \param addr: address
    \param size: full size
    \retval none
*/
__STATIC_INLINE void direct_read(HANDLE process, void* addr, unsigned int size)
{
    svc_call(SVC_DIRECT_READ, (unsigned int)process, (unsigned int)addr, (unsigned int)size);
}

/**
    \brief do direct write
    \param process: handle of process, enabled directed IO
    \param addr: address
    \param size: full size
    \retval none
*/
__STATIC_INLINE void direct_write(HANDLE process, void* addr, unsigned int size)
{
    svc_call(SVC_DIRECT_WRITE, (unsigned int)process, (unsigned int)addr, (unsigned int)size);
}

/** \} */ // end of event group

#endif // DIRECT_H
