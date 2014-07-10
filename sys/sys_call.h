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
    if (ipc_read_ms(&ipc, 0) && ipc.cmd == cmd)
        return true;
    if (ipc.cmd == IPC_UNKNOWN)
        error(ERROR_NOT_SUPPORTED);
    if (ipc.cmd == IPC_INVALID_PARAM)
        error(ERROR_NOT_SUPPORTED);
    return false;
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
    if (ipc_read_ms(&ipc, 0) && ipc.cmd == cmd)
        return ipc.param1;
    if (ipc.cmd == IPC_UNKNOWN)
        error(ERROR_NOT_SUPPORTED);
    if (ipc.cmd == IPC_INVALID_PARAM)
        error(ERROR_NOT_SUPPORTED);
    return 0;
}

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

/** \} */ // end of sys group

#endif // SYS_CALL_H
