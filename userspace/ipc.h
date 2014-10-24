/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IPC_H
#define IPC_H

#include "lib/types.h"

typedef enum {
    IPC_COMMON = 0x0,
    IPC_CALL_ERROR,                                     //!< IPC caller returned error. Check value in param1. Never answer to this IPC
    IPC_INVALID_PARAM,
    IPC_PING,
    IPC_STREAM_WRITE,                                   //!< Sent by kernel when stream write is complete. Param1: write size, Param2: none

    IPC_SYSTEM = 0x1000,
    IPC_USER = 0x10000
} IPCS;

typedef struct {
    HANDLE process;
    unsigned int cmd;
    unsigned int param1;
    unsigned int param2;
    unsigned int param3;
} IPC;

/** \addtogroup IPC IPC
    IPC is sending messages from sender process to receiver

    \{
 */

#include "svc.h"
#include "error.h"
#include "lib/time.h"

/**
    \brief post IPC
    \param ipc: IPC structure
    \retval none
*/
__STATIC_INLINE void ipc_post(IPC* ipc)
{
    svc_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
}

/**
    \brief post IPC, inline version
    \param process: receiver process
    \param cmd: command
    \param param1: cmd-specific param1
    \param param2: cmd-specific param2
    \param param3: cmd-specific param3
    \retval none
*/
__STATIC_INLINE void ipc_post_inline(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.process = process;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    svc_call(SVC_IPC_POST, (unsigned int)&ipc, 0, 0);
}

/**
    \brief post IPC
    \param process: IPC receiver
    \param code: error code
    \retval none
*/
__STATIC_INLINE void ipc_post_error(HANDLE process, int code)
{
    IPC ipc;
    ipc.process = process;
    ipc.cmd = IPC_CALL_ERROR;
    ipc.param1 = code;
    svc_call(SVC_IPC_POST, (unsigned int)&ipc, 0, 0);
}

/**
    \brief post IPC or error if any
    \param ipc: IPC structure
    \retval none
*/
__STATIC_INLINE void ipc_post_or_error(IPC* ipc)
{
    if (get_last_error() != ERROR_OK)
    {
        ipc->param1 = get_last_error();
        ipc->cmd = IPC_CALL_ERROR;
    }
    svc_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
}

/**
    \brief post IPC
    \details This version must be called for IRQ context
    \param ipc: IPC structure
    \retval none
*/
__STATIC_INLINE void ipc_ipost(IPC* ipc)
{
    __GLOBAL->svc_irq(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
}

/**
    \brief read IPC
    \param ipc: ipc
    \param time: timeout
    \param wait_process: process, from where receive IPC. If set, returns only IPC from desired process
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool ipc_read(IPC* ipc, TIME* timeout, HANDLE wait_process)
{
    svc_call(SVC_IPC_READ, (unsigned int)ipc, (unsigned int)timeout, wait_process);
    return get_last_error() == ERROR_OK;
}

/**
    \brief read IPC in ms units
    \param ipc: ipc
    \param ms: timeout in ms
    \param wait_process: process, from where receive IPC. If set, returns only IPC from desired process
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool ipc_read_ms(IPC* ipc, unsigned int ms, HANDLE wait_process)
{
    TIME timeout;
    ms_to_time(ms, &timeout);
    return ipc_read(ipc, &timeout, wait_process);
}

/**
    \brief read IPC in us units
    \param ipc: ipc
    \param us: timeout in us
    \param wait_process: process, from where receive IPC. If set, returns only IPC from desired process
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool ipc_read_us(IPC* ipc, unsigned int us, HANDLE wait_process)
{
    TIME timeout;
    us_to_time(us, &timeout);
    return ipc_read(ipc, &timeout, wait_process);
}

/**
    \brief post IPC, wait for IPC, peek IPC
    \param ipc: IPC structure to send and receive
    \param timeout: pointer to TIME structure
    \retval none
*/
__STATIC_INLINE void ipc_call(IPC* ipc, TIME* timeout)
{
    svc_call(SVC_IPC_CALL, (unsigned int)ipc, (unsigned int)timeout, 0);
}

/**
    \brief post IPC, wait for IPC, peek IPC in ms units
    \param ipc: IPC structure to send and receive
    \param ms: timeout in ms
    \retval none
*/
__STATIC_INLINE void ipc_call_ms(IPC* ipc, unsigned int ms)
{
    TIME timeout;
    ms_to_time(ms, &timeout);
    ipc_call(ipc, &timeout);
}

/**
    \brief post IPC, wait for IPC, peek IPC in us units
    \param ipc: IPC structure to send and receive
    \param us: timeout in us
    \retval none
*/
__STATIC_INLINE void ipc_call_us(IPC* ipc, unsigned int us)
{
    TIME timeout;
    us_to_time(us, &timeout);
    ipc_call(ipc, &timeout);
}

/**
    \brief call to process.
    \param ipc: \ref ipc to post/read.
    \retval true on success
*/

__STATIC_INLINE bool call(IPC* ipc)
{
    unsigned int cmd = ipc->cmd;
    ipc_call_ms(ipc, 0);
    if (ipc->cmd == cmd)
        return true;
    if (ipc->cmd == IPC_CALL_ERROR)
        error(ipc->param1);
    return false;
}

/**
    \brief call to process with no return value
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval true on success
*/

__STATIC_INLINE bool ack(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    return call(&ipc);
}

/**
    \brief get value from process
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval param1
*/

__STATIC_INLINE unsigned int get(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    if (call(&ipc))
        return ipc.param1;
    return INVALID_HANDLE;
}

/** \} */ // end of sem group

#endif // IPC_H
