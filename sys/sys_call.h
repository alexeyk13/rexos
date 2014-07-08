/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_CALL_H
#define SYS_CALL_H

#include "../userspace/ipc.h"
#include "../userspace/cc_macro.h"
#include "../userspace/error.h"
#include "sys.h"

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
    if (ipc_read_ms(ipc, 0))
    {
        if (ipc->cmd == IPC_UNKNOWN || ipc->cmd == IPC_INVALID_PARAM)
            error(ERROR_NOT_SUPPORTED);
    }
    return get_last_error() == ERROR_OK;
}

/**
    \brief post ipc to system without answer
    \param cmd: \ref command to post
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval none
*/

__STATIC_INLINE void sys_post(unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.process = __HEAP->system;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    ipc_post(&ipc);
}

/**
    \brief call to process.
    \param ipc: \ref ipc to post/read.
    \retval true on success
*/

__STATIC_INLINE bool call(IPC* ipc)
{
    if (ipc_read_ms(ipc, 0))
    {
        if (ipc->cmd == IPC_UNKNOWN || ipc->cmd == IPC_INVALID_PARAM)
            error(ERROR_NOT_SUPPORTED);
    }
    return get_last_error() == ERROR_OK;
}

/**
    \brief post IPC to process
    \param cmd: \ref command to post
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval none
*/

__STATIC_INLINE void post(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.process = process;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    ipc_post(&ipc);
}



/** \} */ // end of sys group

#endif // SYS_CALL_H
