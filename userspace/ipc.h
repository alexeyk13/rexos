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

/**
    \brief post IPC
    \param ipc: IPC structure
    \retval true on success
*/
__STATIC_INLINE bool ipc_post(IPC* ipc)
{
    svc_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief post IPC
    \param process: IPC receiver
    \param code: error code
    \retval true on success
*/
__STATIC_INLINE bool ipc_post_error(HANDLE process, int code)
{
    IPC ipc;
    ipc.process = process;
    ipc.cmd = IPC_CALL_ERROR;
    ipc.param1 = code;
    svc_call(SVC_IPC_POST, (unsigned int)&ipc, 0, 0);
    return get_last_error() == ERROR_OK;
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
    \retval true on success
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
    \retval true on success
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
    \retval true on success
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
    \retval true on success
*/
__STATIC_INLINE bool ipc_call(IPC* ipc, TIME* timeout)
{
    svc_call(SVC_IPC_CALL, (unsigned int)ipc, (unsigned int)timeout, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief post IPC, wait for IPC, peek IPC in ms units
    \param ipc: IPC structure to send and receive
    \param ms: timeout in ms
    \retval true on success
*/
__STATIC_INLINE bool ipc_call_ms(IPC* ipc, unsigned int ms)
{
    TIME timeout;
    ms_to_time(ms, &timeout);
    return ipc_call(ipc, &timeout);
}

/**
    \brief post IPC, wait for IPC, peek IPC in us units
    \param ipc: IPC structure to send and receive
    \param us: timeout in us
    \retval true on success
*/
__STATIC_INLINE bool ipc_call_us(IPC* ipc, unsigned int us)
{
    TIME timeout;
    us_to_time(us, &timeout);
    return ipc_call(ipc, &timeout);
}


/** \} */ // end of sem group

#endif // IPC_H
