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

static inline bool tlss_rx_change_cypher(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned short len)
{
    printd("TODO: change cypher\n");
    dump(data, len);
    return false;
}

static inline bool tlss_rx_alert(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned short len)
{
    printd("TODO: alert\n");
    dump(data, len);
    return false;
}

static inline bool tlss_rx_client_hello(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned short len)
{
    printd("TODO: ClientHello\n");
    dump(data, len);
    return false;
}

static inline bool tlss_rx_handshakes(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned short len)
{
    unsigned short offset, len_cur;
    TLS_HANDSHAKE* handshake;
    void* data_cur;
    bool alert;
    //iterate through handshake messages
    for (offset = 0; offset < len; offset += len_cur + sizeof(TLS_HANDSHAKE))
    {
        if (len - offset < sizeof(TLS_HANDSHAKE))
        {
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_UNEXPECTED_MESSAGE);
            return true;
        }
        handshake = (TLS_HANDSHAKE*)((uint8_t*)data + offset);
        len_cur = be2short(handshake->message_length_be);
        if ((len_cur + sizeof(TLS_HANDSHAKE) > len - offset) || (handshake->reserved))
        {
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_UNEXPECTED_MESSAGE);
            return true;
        }
        data_cur = (uint8_t*)data + offset + sizeof(TLS_HANDSHAKE);
        switch (handshake->message_type)
        {
        case TLS_HANDSHAKE_CLIENT_HELLO:
            alert = tlss_rx_client_hello(tlss, tcb, data_cur, len_cur);
            break;
        //TODO: not sure about others this time
//        TLS_HANDSHAKE_HELLO_REQUEST:
//        TLS_HANDSHAKE_CLIENT_HELLO,
//        TLS_HANDSHAKE_CERTIFICATE = 11,
//        TLS_HANDSHAKE_SERVER_KEY_EXCHANGE,
//        TLS_HANDSHAKE_CERTIFICATE_REQUEST,
//        TLS_HANDSHAKE_SERVER_HELLO_DONE,
//        TLS_HANDSHAKE_CERTIFICATE_VERIFY,
//        TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE,
//        TLS_HANDSHAKE_FINISHED = 20
        default:
#if (TLS_DEBUG)
            printf("TLS: unexpected handshake type: %d\n", handshake->message_type);
#endif //TLS_DEBUG
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_UNEXPECTED_MESSAGE);
            alert = true;
        }
        if (alert)
            return true;
    }
    return false;
}

static inline bool tlss_rx_app(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned short len)
{
    printd("TODO: app\n");
    dump(data, len);
    return false;
}

static inline void tlss_tcp_rx(TLSS* tlss, HANDLE handle)
{
    bool alert;
    TLS_RECORD* rec;
    unsigned short len;
    void* data;
    TLSS_TCB* tcb = tlss_tcb_find(tlss, handle);
    alert = true;
    do {
        //closed before
        if (tcb == NULL)
        {
            alert = false;
            break;
        }
        //Empty records disabled by TLS
        if (tlss->io->data_size <= sizeof(TLS_RECORD))
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
        if (len > tlss->io->data_size - sizeof(TLS_RECORD))
        {
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_UNEXPECTED_MESSAGE);
            break;
        }
        data = (uint8_t*)io_data(tlss->io) + sizeof(TLS_RECORD);
        //TODO: check state before
        switch (rec->content_type)
        {
        case TLS_CONTENT_CHANGE_CYPHER:
            alert = tlss_rx_change_cypher(tlss, tcb, data, len);
            break;
        case TLS_CONTENT_ALERT:
            alert = tlss_rx_alert(tlss, tcb, data, len);
            break;
        case TLS_CONTENT_HANDSHAKE:
            alert = tlss_rx_handshakes(tlss, tcb, data, len);
            break;
        case TLS_CONTENT_APP:
            alert = tlss_rx_app(tlss, tcb, data, len);
            break;
        default:
#if (TLS_DEBUG)
            printf("TLS: unexpected message type: %d\n", rec->content_type);
#endif //TLS_DEBUG
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, TLS_ALERT_UNEXPECTED_MESSAGE);
        }

        //TODO: process after
    } while (false);
    if (!alert)
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
        tlss_tcp_rx(tlss, (HANDLE)ipc->param1);
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
