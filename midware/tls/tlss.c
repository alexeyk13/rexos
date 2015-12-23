/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tlss.h"
#include "sys_config.h"
#include "../../userspace/process.h"
#include "../../userspace/stdio.h"
#include "../../userspace/sys.h"

void tlss_main();

typedef struct {
    HANDLE tcpip;
} TLSS;

const REX __TLSS = {
    //name
    "TLS Server",
    //size
    TLS_PROCESS_SIZE,
    //priority - midware priority
    TLS_PROCESS_PRIORITY,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    tlss_main
};

static inline void tlss_init(TLSS* tlss)
{
    tlss->tcpip = INVALID_HANDLE;
}

static inline void tlss_open(TLSS* tlss, HANDLE tcpip)
{
    if (tlss->tcpip != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tlss->tcpip = tcpip;
}

static inline void tlss_close(TLSS* tlss)
{
    if (tlss->tcpip == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //TODO: flush & close all handles
    tlss->tcpip = INVALID_HANDLE;
}

static inline void tlss_request(TLSS* tlss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        tlss_open(tlss, (HANDLE)ipc->param2);
        break;
    case IPC_CLOSE:
        tlss_close(tlss);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void tlss_tcp_request(TLSS* tlss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
///    case IPC_OPEN:
        //TODO:
///    case IPC_CLOSE:
        //TODO:
///    case IPC_READ:
        //TODO:
///    case IPC_WRITE:
        //TODO:
    default:
        printf("got from tcp\n");
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void tlss_user_request(TLSS* tlss, IPC* ipc)
{
    //TODO:
//    TCP_LISTEN
//    TCP_CLOSE_LISTEN
//    TCP_GET_REMOTE_ADDR
//    TCP_GET_REMOTE_PORT
//    TCP_GET_LOCAL_PORT

    switch (HAL_ITEM(ipc->cmd))
    {
///    case IPC_OPEN:
        //TODO:
///    case IPC_CLOSE:
        //TODO:
///    case IPC_READ:
        //TODO:
///    case IPC_WRITE:
        //TODO:
    default:
        printf("tls user request\n");
        error(ERROR_NOT_SUPPORTED);
    }
}

void tlss_main()
{
    IPC ipc;
    TLSS tlss;
    tlss_init(&tlss);
#if (TLS_DEBUG)
    open_stdout();
#endif //HS_DEBUG
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_TLS:
            tlss_request(&tlss, &ipc);
            break;
        case HAL_TCP:
            if (ipc.process == tlss.tcpip)
                tlss_tcp_request(&tlss, &ipc);
            else
                tlss_user_request(&tlss, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}
