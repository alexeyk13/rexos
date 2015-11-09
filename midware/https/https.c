/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "https.h"
#include "../../userspace/process.h"
#include "../../userspace/ipc.h"
#include "../../userspace/error.h"
#include "sys_config.h"

typedef struct {
    HANDLE handle, tcpip, process;
} HTTPS;

static inline void https_init(HTTPS* https)
{
    https->process = INVALID_HANDLE;
}

static inline void https_open(HTTPS* https, unsigned int handle, HANDLE tcpip, HANDLE process)
{
    if (https->process != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    https->handle = handle;
    https->tcpip = tcpip;
    https->process = process;
    //TODO:
}

static inline void https_request(HTTPS* https, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        https_open(https, ipc->param1, ipc->param2, ipc->process);
        break;
    case IPC_CLOSE:
        //TODO:
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void https_main()
{
    IPC ipc;
    HTTPS https;
    https_init(&https);
#if (HTTPS_DEBUG)
    open_stdout();
#endif //HTTPS_DEBUG
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_HTTP:
            https_request(&https, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}

