/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IPC_H
#define IPC_H

#include "lib/types.h"

typedef struct {
    HANDLE process;
    unsigned int cmd;
    int param1;
    int param2;
} IPC;

/** \addtogroup IPC IPC
    IPC is sending messages from sender process to receiver

    \{
 */

#include "sys.h"
#include "error.h"

/**
    \brief post IPC
    \param ipc: IPC structure
    \retval true on success
*/
__STATIC_INLINE bool ipc_post(IPC* ipc)
{
    sys_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
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
    \brief peek IPC
    \param ipc: IPC structure to receive
    \param wait_process: process, from where receive IPC. If set, returns only IPC from desired process
    \retval true on success
*/
__STATIC_INLINE bool ipc_peek(IPC* ipc, HANDLE wait_process)
{
    sys_call(SVC_IPC_PEEK, (unsigned int)ipc, wait_process, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief wait for IPC
    \param timeout: pointer to TIME structure
    \param wait_process: process, from where receive IPC. If set, returns only IPC from desired process
    \retval true on success
*/
__STATIC_INLINE bool ipc_wait(TIME* timeout, HANDLE wait_process)
{
    sys_call(SVC_IPC_WAIT, (unsigned int)timeout, wait_process, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief wait for IPC in ms units
    \param ms: timeout in ms
    \param wait_process: process, from where receive IPC. If set, returns only IPC from desired process
    \retval true on success
*/
__STATIC_INLINE bool ipc_wait_ms(unsigned int ms, HANDLE wait_process)
{
    TIME timeout;
    ms_to_time(ms, &timeout);
    return ipc_wait(&timeout, wait_process);
}


/**
    \brief wait for IPC in us units
    \param us: timeout in us
    \param wait_process: process, from where receive IPC. If set, returns only IPC from desired process
    \retval true on success
*/
__STATIC_INLINE bool ipc_wait_us(unsigned int us, HANDLE wait_process)
{
    TIME timeout;
    us_to_time(us, &timeout);
    return ipc_wait(&timeout, wait_process);
}

/**
    \brief post IPC, wait for IPC, peek IPC
    \param ipc: IPC structure to send and receive
    \param timeout: pointer to TIME structure
    \retval true on success
*/
__STATIC_INLINE bool ipc_read(IPC* ipc, TIME* timeout)
{
    HANDLE wait_process = ipc->process;
    sys_call(SVC_IPC_POST_WAIT, (unsigned int)ipc, (unsigned int)timeout, 0);
    if (get_last_error() == ERROR_OK)
        sys_call(SVC_IPC_PEEK, (unsigned int)ipc, wait_process, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief post IPC, wait for IPC, peek IPC in ms units
    \param ipc: IPC structure to send and receive
    \param ms: timeout in ms
    \retval true on success
*/
__STATIC_INLINE bool ipc_read_ms(IPC* ipc, unsigned int ms)
{
    TIME timeout;
    ms_to_time(ms, &timeout);
    return ipc_read(ipc, &timeout);
}

/**
    \brief post IPC, wait for IPC, peek IPC in us units
    \param ipc: IPC structure to send and receive
    \param us: timeout in us
    \retval true on success
*/
__STATIC_INLINE bool ipc_read_us(IPC* ipc, unsigned int us)
{
    TIME timeout;
    us_to_time(us, &timeout);
    return ipc_read(ipc, &timeout);
}


/** \} */ // end of sem group

#endif // IPC_H
