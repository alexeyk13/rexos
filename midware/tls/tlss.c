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
#include "../../userspace/io.h"
#include "../../userspace/so.h"
#include "../../userspace/tcp.h"

void tlss_main();

typedef struct {
    int stub;
} TLS_TCB;

typedef struct {
    HANDLE tcpip;
    IO* io;
    SO tcbs;
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
    tlss->io = NULL;
    so_create(&tlss->tcbs, sizeof(TLS_TCB), 1);
}

static inline void tlss_open(TLSS* tlss, HANDLE tcpip)
{
    if (tlss->tcpip != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tlss->io = io_create(TLS_IO_SIZE + sizeof(TCP_STACK));
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
    io_destroy(tlss->io);
    tlss->io = NULL;
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

static inline void tlss_tcp_open(TLSS* tlss, HANDLE handle)
{
#if (TLS_DEBUG)
///    tcp_get_remote_addr(hss->tcpip, handle, &session->remote_addr);
    printf("TLS: new session from ");
///    ip_print(&session->remote_addr);
    printf("\n");
#endif //TLS_DEBUG
    //TODO: create tcb, check if not busy
    io_reset(tlss->io);
    tcp_read(tlss->tcpip, handle, tlss->io, TLS_IO_SIZE);
}

static inline void tlss_tcp_read_complete(TLSS* tlss, HANDLE handle)
{
    printd("TLS: RX\n");
    dump(io_data(tlss->io), tlss->io->data_size);
    ((char*)io_data(tlss->io))[tlss->io->data_size] = 0x00;
    printd("TLS: data: %s\n", io_data(tlss->io));
}

static inline void tlss_tcp_request(TLSS* tlss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        tlss_tcp_open(tlss, (HANDLE)ipc->param1);
        break;
///    case IPC_CLOSE:
        //TODO:
    case IPC_READ:
        tlss_tcp_read_complete(tlss, (HANDLE)ipc->param1);
        break;
///    case IPC_WRITE:
        //TODO:
    default:
        printf("got from tcp\n");
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void tlss_user_listen(TLSS* tlss, IPC* ipc)
{
    //just forward to tcp
    if (tlss->tcpip == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //just forward to tcp
    ipc->param2 = tcp_listen(tlss->tcpip, ipc->param1);
}

static inline void tlss_user_close_listen(TLSS* tlss, IPC* ipc)
{
    //just forward to tcp
    if (tlss->tcpip == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //just forward to tcp
    tcp_close_listen(tlss->tcpip, ipc->param1);
}

static inline void tlss_user_request(TLSS* tlss, IPC* ipc)
{
    //TODO:
//    TCP_GET_REMOTE_ADDR
//    TCP_GET_REMOTE_PORT
//    TCP_GET_LOCAL_PORT

    switch (HAL_ITEM(ipc->cmd))
    {
    case TCP_LISTEN:
        tlss_user_listen(tlss, ipc);
        break;
    case TCP_CLOSE_LISTEN:
        tlss_user_close_listen(tlss, ipc);
        break;
///    case IPC_CLOSE:
        //TODO:
///    case IPC_READ:
        //TODO:
///    case IPC_WRITE:
        //TODO:
///    case IPC_FLUSH:
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
