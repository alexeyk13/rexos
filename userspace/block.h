/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef BLOCK_H
#define BLOCK_H

#include "types.h"
#include "cc_macro.h"
#include "svc.h"
#include "error.h"
#include "ipc.h"

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
    \retval ptr on success
*/
__STATIC_INLINE void* block_open(HANDLE block)
{
    void* ptr;
    svc_call(SVC_BLOCK_OPEN, (unsigned int)block, (unsigned int)&ptr, 0);
    return ptr;
}

/**
    \brief get block data size. Process must have access to block
    \param block: block handle
    \retval data size
*/
__STATIC_INLINE unsigned int block_get_size(HANDLE block)
{
    unsigned int size;
    svc_call(SVC_BLOCK_GET_SIZE, (unsigned int)block, (unsigned int)(&size), 0);
    return size;
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
    \brief send block to another process + send custom IPC in one kernel call
    \param block: block handle
    \param receiver: receiver process
    \param ipc: custom ipc
    \retval none
*/
__STATIC_INLINE void block_send_ipc(HANDLE block, HANDLE receiver, IPC* ipc)
{
    svc_call(SVC_BLOCK_SEND_IPC, (unsigned int)block, (unsigned int)receiver, (unsigned int)ipc);
}

/**
    \brief send block to another process + send custom IPC in one kernel call. Inline mode
    \param block: block handle
    \param receiver: receiver process
    \param cmd: command to send
    \param param1: custom param1
    \param param2: custom param2
    \param param3: custom param3
    \retval none
*/
__STATIC_INLINE void block_send_ipc_inline(HANDLE block, HANDLE receiver, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.process = receiver;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    svc_call(SVC_BLOCK_SEND_IPC, (unsigned int)block, (unsigned int)receiver, (unsigned int)&ipc);
}

/**
    \brief send block to another process, isr version
    \param block: block handle
    \param receiver: receiver process
    \retval none
*/
__STATIC_INLINE void block_isend(HANDLE block, HANDLE receiver)
{
    __GLOBAL->svc_irq(SVC_BLOCK_SEND, (unsigned int)block, (unsigned int)receiver, 0);
}

/**
    \brief send block to another process + send custom IPC in one kernel call, isr mode
    \param block: block handle
    \param receiver: receiver process
    \param ipc: custom ipc
    \retval none
*/
__STATIC_INLINE void block_isend_ipc(HANDLE block, HANDLE receiver, IPC* ipc)
{
    __GLOBAL->svc_irq(SVC_BLOCK_SEND_IPC, (unsigned int)block, (unsigned int)receiver, (unsigned int)ipc);
}

/**
    \brief send block to another process + send custom IPC in one kernel call. Inline mode, isr mode
    \param block: block handle
    \param receiver: receiver process
    \param cmd: command to send
    \param param1: custom param1
    \param param2: custom param2
    \param param3: custom param3
    \retval none
*/
__STATIC_INLINE void block_isend_ipc_inline(HANDLE block, HANDLE receiver, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.process = receiver;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    __GLOBAL->svc_irq(SVC_BLOCK_SEND_IPC, (unsigned int)block, (unsigned int)receiver, (unsigned int)&ipc);
}

/**
    \brief return block to owner
    \param block: block handle
    \retval none
*/
__STATIC_INLINE void block_return(HANDLE block)
{
    svc_call(SVC_BLOCK_RETURN, (unsigned int)block, 0, 0);
}

/**
    \brief return block to owner, isr version
    \param block: block handle
    \retval none
*/
__STATIC_INLINE void block_ireturn(HANDLE block)
{
    __GLOBAL->svc_irq(SVC_BLOCK_RETURN, (unsigned int)block, 0, 0);
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
