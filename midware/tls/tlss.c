/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tlss.h"
#include "tls_private.h"
#include "sys_config.h"
#include "../../userspace/process.h"
#include "../../userspace/stdio.h"
#include "../../userspace/sys.h"
#include "../../userspace/io.h"
#include "../../userspace/so.h"
#include "../../userspace/tcp.h"
#include "../../userspace/endian.h"

void tlss_main();

typedef enum {
    //TODO: more states to go
    TLSS_STATE_CLIENT_HELLO,
    TLSS_STATE_SERVER_HELLO,
    TLSS_STATE_ESTABLISHED,
    TLSS_STATE_CLOSING
} TLSS_STATE;

typedef struct {
    HANDLE handle;
    IO* rx;
    IO* tx;
    TLSS_STATE state;
} TLSS_TCB;

typedef struct {
    HANDLE tcpip, user;
    IO* io;
    IO* tx;
    bool tx_busy;
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

static HANDLE tlss_create_tcb(TLSS* tlss, HANDLE handle)
{
    TLSS_TCB* tcb;
    HANDLE tcb_handle;
    //TODO: multiple sessions support
    if (so_count(&tlss->tcbs))
        return INVALID_HANDLE;
    tcb_handle = so_allocate(&tlss->tcbs);
    if (tcb_handle == INVALID_HANDLE)
        return INVALID_HANDLE;
    tcb = so_get(&tlss->tcbs, tcb_handle);
    tcb->handle = handle;
    tcb->rx = tcb->tx = NULL;
    tcb->state = TLSS_STATE_CLIENT_HELLO;
    return tcb_handle;
}

//TODO: tlss_destroy_tcb

static TLSS_TCB* tlss_tcb_find(TLSS* tlss, HANDLE handle)
{
    TLSS_TCB* tcb;
    HANDLE tcb_handle;
    for (tcb_handle = so_first(&tlss->tcbs); tcb_handle != INVALID_HANDLE; tcb_handle = so_next(&tlss->tcbs, tcb_handle))
    {
        tcb = so_get(&tlss->tcbs, tcb_handle);
        if (tcb->handle == handle)
            return tcb;
    }
    return NULL;
}

static void tlss_rx(TLSS* tlss)
{
    HANDLE tcb_handle;
    TLSS_TCB* tcb;
    if (tlss->tcpip == INVALID_HANDLE)
        return;
    //TODO: read from listen handle
    tcb_handle = so_first(&tlss->tcbs);
    //no active listen handles
    if (tcb_handle == INVALID_HANDLE)
        return;
    io_reset(tlss->io);
    tcb = so_get(&tlss->tcbs, tcb_handle);
    tcp_read(tlss->tcpip, tcb->handle, tlss->io, TLS_IO_SIZE);
}

static void tlss_tx_alert(TLSS* tlss, TLSS_TCB* tcb, TLS_ALERT_LEVEL alert_level, TLS_ALERT_DESCRIPTION alert_description)
{
    printd("TODO: TLS alert\n");
}

static inline void tlss_init(TLSS* tlss)
{
    tlss->tcpip = INVALID_HANDLE;
    tlss->user = INVALID_HANDLE;
    tlss->io = NULL;
    tlss->tx = NULL;
    tlss->tx_busy = false;
    so_create(&tlss->tcbs, sizeof(TLSS_TCB), 1);
}

static inline void tlss_open(TLSS* tlss, HANDLE tcpip)
{
    if (tlss->tcpip != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tlss->io = io_create(TLS_IO_SIZE + sizeof(TCP_STACK));
    tlss->tx = io_create(TLS_IO_SIZE + sizeof(TCP_STACK));
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
    io_destroy(tlss->tx);
    tlss->io = NULL;
    tlss->tx = NULL;
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
    HANDLE tcb_handle = tlss_create_tcb(tlss, handle);
    if (tcb_handle == INVALID_HANDLE)
    {
        tcp_close(tlss->tcpip, handle);
        return;
    }
#if (TLS_DEBUG)
///    tcp_get_remote_addr(hss->tcpip, handle, &session->remote_addr);
    printf("TLS: new session from ");
///    ip_print(&session->remote_addr);
    printf("\n");
#endif //TLS_DEBUG
    tlss_rx(tlss);
}

static inline void tlss_tcp_read_complete(TLSS* tlss, HANDLE handle)
{
    bool read_next;
    TLS_RECORD_HEADER* rec;
    unsigned short len;
    TLSS_TCB* tcb = tlss_tcb_find(tlss, handle);
    read_next = false;
    do {
        //closed before
        if (tcb == NULL)
        {
            read_next = true;
            break;
        }
        if (tlss->io->data_size < sizeof(TLS_RECORD_HEADER))
        {
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_UNEXPECTED_MESSAGE);
            break;
        }
        rec = io_data(tlss->io);
        //check TLS 1.0 - 1.2
        if ((rec->version.major != 3) || (rec->version.minor == 0) || (rec->version.minor > 3))
        {
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_PROTOCOL_VERSION);
            break;
        }
        len = be2short(rec->record_length_be);
        if (len > tlss->io->data_size - sizeof(TLS_RECORD_HEADER))
        {
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_UNEXPECTED_MESSAGE);
            break;
        }
        tlss->io->data_size = len + sizeof(TLS_RECORD_HEADER);




        printd("TLS: RX\n");
        dump(io_data(tlss->io), tlss->io->data_size);
        ((char*)io_data(tlss->io))[tlss->io->data_size] = 0x00;
        printd("TLS: data: %s\n", io_data(tlss->io));

    } while (false);
    if (read_next)
        tlss_rx(tlss);
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
    if (tlss->user != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tlss->user = ipc->process;
    //just forward to tcp
    ipc->param2 = tcp_listen(tlss->tcpip, ipc->param1);
}

static inline void tlss_user_close_listen(TLSS* tlss, IPC* ipc)
{
    //just forward to tcp
    if (tlss->user == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    tlss->user = INVALID_HANDLE;
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
