/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "hss.h"
#include "../../userspace/ipc.h"
#include "../../userspace/error.h"
#include "../../userspace/ip.h"
#include "../../userspace/tcp.h"
#include "../../userspace/stdio.h"
#include "../../userspace/sys.h"
#include "sys_config.h"

void hss_main();

typedef enum {
    HSS_STATE_IDLE,
    HSS_STATE_RX_HEAD,
    HSS_STATE_RX_DATA,
    HSS_STATE_TX
} HSS_STATE;

typedef struct {
    IO* io;
    HANDLE conn;
#if (HS_DEBUG)
    IP remote_addr;
#endif //HS_DEBUG
    //TODO: session timer
} HSS_SESSION;

typedef struct {
    HANDLE tcpip, process, listener;
    IO* user_io;
    unsigned int content_size;
    HSS_STATE state;
    HSS_SESSION* active_session;

    //only one session for now
    HSS_SESSION session;
} HSS;

const REX __HSS = {
    //name
    "HTTP Server",
    //size
    HS_PROCESS_SIZE,
    //priority - midware priority
    HS_PROCESS_PRIORITY,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    hss_main
};

static inline void hss_init(HSS* hss)
{
    hss->process = INVALID_HANDLE;
    hss->user_io = NULL;
    hss->state = HSS_STATE_IDLE;
    hss->active_session = NULL;

    //TODO: Multiple session support
    hss->session.io = NULL;
}

static inline HSS_SESSION* hss_create_session(HSS* hss)
{
    //TODO: Multiple session support
    if (hss->session.io != NULL)
    {
        error(ERROR_TOO_MANY_HANDLES);
#if (HS_DEBUG)
        printf("HS: Too many connections\n");
#endif //HS_DEBUG
        return NULL;
    }
    //TODO: allocation queue (tcpip style)
    hss->session.io = io_create(HS_IO_SIZE + sizeof(TCP_STACK));
    if (hss->session.io == NULL)
        return NULL;
    return &hss->session;
}

static void hss_destroy_session(HSS* hss, HSS_SESSION* session)
{
    //TODO: allocation queue (tcpip style)
    io_destroy(session->io);
    session->io = NULL;
}

static void hss_flush_session(HSS* hss, HSS_SESSION* session)
{
    hss->active_session = NULL;
    hss->state = HSS_STATE_IDLE;
    if (hss->user_io)
        io_reset(hss->user_io);

    //TODO: Multiple session support, switch to next session (if some data pending)
    io_reset(session->io);
}

static inline void hss_open(HSS* hss, uint16_t port, HANDLE tcpip, HANDLE process)
{
    if (hss->process != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    hss->listener = tcp_listen(hss->tcpip, port);
    if (hss->listener == INVALID_HANDLE)
        return;
    hss->tcpip = tcpip;
    hss->process = process;
}

static inline void hss_close(HSS* hss, HANDLE process)
{
    if (process != hss->process)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    if (hss->process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //TODO: Multiple session support
    hss_destroy_session(hss, &hss->session);
    tcp_close_listen(hss->tcpip, hss->listener);
    hss->process = INVALID_HANDLE;
}

static inline void hss_request(HSS* hss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        hss_open(hss, ipc->param1, ipc->param2, ipc->process);
        break;
    case IPC_CLOSE:
        hss_close(hss, ipc->process);
        break;
    case IPC_READ:
        //TODO: give buffer
///        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void hss_open_session(HSS* hss, HANDLE handle)
{
    HSS_SESSION* session = hss_create_session(hss);

    if (session == NULL)
    {
        tcp_close(hss->tcpip, handle);
        return;
    }
    session->conn = handle;

#if (HS_DEBUG)
    tcp_get_remote_addr(hss->tcpip, handle, &session->remote_addr);
    printf("HS: new session from ");
    ip_print(&session->remote_addr);
    printf("\n");
#endif //HS_DEBUG

    io_reset(session->io);
    tcp_read(hss->tcpip, session->conn, session->io, HS_IO_SIZE);
}

static inline void hss_close_session(HSS* hss, HSS_SESSION* session)
{
#if (HS_DEBUG)
    printf("HS: closed session from ");
    ip_print(&session->remote_addr);
    printf("\n");
#endif //HS_DEBUG

    hss_flush_session(hss, session);
    hss_destroy_session(hss, session);
}

static inline void hss_rx(HSS* hss, HSS_SESSION* session, int size)
{
///    TCP_STACK* tcp_stack;
    //we don't need TCP stack at all
///    io_pop(session->io, sizeof(TCP_STACK));

    if (size < 0)
    {
        //only reason of error is connection closed
        hss_flush_session(hss, session);
        return;
    }

    //TODO: append buffers till head is received
    //TODO: return here if not

    //no buffer? Nothing to do
    if (hss->user_io == NULL)
        return;

    //no active session? Make this active
    if (hss->active_session == NULL)
        hss->active_session = session;

    //this session is active? If no, wait for buffer
    if (session != hss->active_session)
        return;

    //TODO: depending on state parse request, send to user or read more
/*
    do {
        if (!http_parse_request(app->net.io, &http))
            break;
        if (!http_get_path(&http, &path))
            break;
        //only root path
        if (!http_compare_path(&path, "/"))
        {
            http_set_error(&http, HTTP_RESPONSE_NOT_FOUND);
            break;
        }
        //nothing in head, prepare for response
        http.head.s = NULL;
        http.head.len = 0;
        switch (http.method)
        {
        case HTTP_GET:
            net_get(app, &http);
            break;
        case HTTP_POST:
            net_post(app, &http);
            break;
        default:
            http_set_error(&http, HTTP_RESPONSE_METHOD_NOT_ALLOWED);
        }
    } while (false);

    switch (http.resp)
    {
    case HTTP_RESPONSE_OK:
        printf("200 OK\n");
        break;
    case HTTP_RESPONSE_NOT_FOUND:
        net_not_found(app, &http);
        printf("404 Not found\n");
        break;
    default:
        printf("Error: %d\n", http.resp);
        net_other_error(app, &http);
        break;
    }

    http.body.s = app->net.page_buf;
    http.body.len = strlen(app->net.page_buf);
    http.content_type = HTTP_CONTENT_HTML;

    http_make_response(&http, app->net.io);
    tcp_stack = io_stack(app->net.io);
    tcp_stack->flags = TCP_PSH;
    tcp_write_sync(app->net.tcpip, app->net.connection, app->net.io);

    io_reset(app->net.io);
    tcp_read(app->net.tcpip, app->net.connection, app->net.io, HTTP_IO_SIZE);*/
}

static inline void hss_tcp_request(HSS* hss, IPC* ipc)
{
    HSS_SESSION* session;
    if (HAL_ITEM(ipc->cmd) == IPC_OPEN)
        hss_open_session(hss, (HANDLE)ipc->param1);
    else
    {
        //TODO: Multiple session support
        session = &hss->session;

        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_CLOSE:
            hss_close_session(hss, session);
            break;
        case IPC_READ:
            hss_rx(hss, session, (int)ipc->param3);
            break;
        case IPC_WRITE:
            //TODO: hss_tx
///            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

void hss_main()
{
    IPC ipc;
    HSS hss;
    hss_init(&hss);
#if (HS_DEBUG)
    open_stdout();
#endif //HS_DEBUG
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_HTTP:
            hss_request(&hss, &ipc);
            break;
        case HAL_TCP:
            hss_tcp_request(&hss, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}

