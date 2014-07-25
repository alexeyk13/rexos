/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef BLOCK_H
#define BLOCK_H

#include "lib/types.h"
#include "cc_macro.h"
#include "svc.h"
#include "error.h"

/** \addtogroup block block
    interprocess block IO

    \{
 */


/**
    \brief creates block
    \param size: size of block. Must be pow of 2 starting from PAGE_SIZE for MPU alignment
    \retval event HANDLE on success. On INVALID_HANDLE check for last error
*/
__STATIC_INLINE HANDLE block_create(unsigned int size)
{
    HANDLE handle;
    svc_call(SVC_BLOCK_CREATE, (unsigned int)&handle, size, 0);
    return handle;
}

/**
    \brief open block
    \param block: block handle
    \retval true on success
*/
__STATIC_INLINE HANDLE block_open(HANDLE block)
{
    svc_call(SVC_BLOCK_OPEN, (unsigned int)block, 0, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief send block to another process
    \param block: block handle
    \param receiver: receiver process
    \retval none
*/
__STATIC_INLINE void block_send(HANDLE block, HANDLE receiver)
{
    svc_call(SVC_BLOCK_SEND, (unsigned int)block, (unsigned int)receiver, 0);
}

/**
    \brief destroy block
    \param block: block handle
    \retval none
*/
__STATIC_INLINE void block_destroy(HANDLE block)
{
    svc_call(SVC_BLOCK_DESTROY, (unsigned int)block, 0, 0);
}

/** \} */ // end of block group

#endif // BLOCK_H
