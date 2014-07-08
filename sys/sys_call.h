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


/** \} */ // end of sys group

#endif // SYS_CALL_H
