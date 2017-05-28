/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "webs.h"
#include "web_parse.h"
#include "web_node.h"
#include "../../userspace/ipc.h"
#include "../../userspace/error.h"
#include "../../userspace/ip.h"
#include "../../userspace/tcp.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/sys.h"
#include "../../userspace/web.h"
#include "../../userspace/array.h"
#include "../../userspace/so.h"
#include <string.h>
#include "sys_config.h"

#if (WEBS_DEBUG_ERRORS) || (WEBS_DEBUG_SESSION) || (WEBS_DEBUG_REQUESTS) || (WEBS_DEBUG_FLOW)
#define WEBS_DEBUG
#endif

void webs_main();

typedef enum {
    WEBS_SESSION_STATE_IDLE,
    WEBS_SESSION_STATE_RX,
    WEBS_SESSION_STATE_PENDING,
    WEBS_SESSION_STATE_REQUEST,
    WEBS_SESSION_STATE_TX
} WEBS_SESSION_STATE;

typedef struct {
    IO* io;
    char* req;
    char* url;
    unsigned int req_size, header_size, status_line_size, data_size, url_size, processed;
    HANDLE conn, node_handle, self;
#if (WEBS_SESSION_TIMEOUT_S)
    HANDLE timer;
#endif //WEBS_SESSION_TIMEOUT_S
#if (WEBS_DEBUG_SESSION)
    IP remote_addr;
#endif //WEBS_DEBUG_SESSION
    WEBS_SESSION_STATE state;
    HTTP_VERSION version;
    WEB_METHOD method;
} WEBS_SESSION;

typedef struct {
    WEB_RESPONSE code;
    char* data;
} WEBS_ERROR;

typedef struct {
    HANDLE tcpip, process, listener;
    bool busy;
    WEB_NODE web_node;

    ARRAY* errors;
    char* generic_error;

    SO sessions;
} WEBS;

#define HTTP_LINE_SIZE                         64

static const char* const __HTTP_REASON100[] = {"Continue",
                                               "Switching Protocols"};
static const char* const __HTTP_REASON200[] = {"OK",
                                               "Created",
                                               "Accepted",
                                               "Non-Authoritative Information",
                                               "No Content",
                                               "Reset Content",
                                               "Partial Content"};
static const char* const __HTTP_REASON300[] = {"Multiple Choices",
                                               "Moved Permanently",
                                               "Found",
                                               "See Other",
                                               "Not Modified",
                                               "Use Proxy",
                                               "",
                                               "Temporary Redirect"};
static const char* const __HTTP_REASON400[] = {"Bad Request",
                                               "Unauthorized",
                                               "Payment Required",
                                               "Forbidden",
                                               "Not Found",
                                               "Method Not Allowed",
                                               "Not Acceptable",
                                                "Proxy Authentication Required",
                                                "Request Timeout",
                                                "Conflict",
                                                "Gone",
                                                "Length Required",
                                                "Precondition Failed",
                                                "Payload Too Large",
                                                "URI Too Long",
                                                "Unsupported Media Type",
                                                "Range Not Satisfiable",
                                                "Expectation Failed",
                                                "",
                                                "",
                                                "",
                                                "",
                                                "",
                                                "",
                                                "",
                                                "",
                                                "Upgrade Required"};
static const char* const __HTTP_REASON500[] =  {"Internal Server Error",
                                                "Not Implemented",
                                                "Bad Gateway",
                                                "Service Unavailable",
                                                "Gateway Timeout",
                                                "HTTP Version Not Supported"};

static const char* const* const __HTTP_REASONS[] =    {__HTTP_REASON100, __HTTP_REASON200, __HTTP_REASON300, __HTTP_REASON400, __HTTP_REASON500};
static const unsigned int __CODE_SIZE[] =             {2, 7, 8, 27, 6};

#define HTTP_STATUS_LINE_SIZE                   15

static inline void webs_init(WEBS* webs)
{
    webs->process = INVALID_HANDLE;
    web_node_create(&webs->web_node);

    webs->busy = false;
    array_create(&webs->errors, sizeof(WEBS_ERROR), 1);
    webs->generic_error = NULL;

    so_create(&webs->sessions, sizeof(WEBS_SESSION), 1);
}

static const char* webs_get_response_text(WEB_RESPONSE code)
{
    unsigned int group, item;
    group = code / 100;
    item = code % 100;
    if (group < 1 || group > 5)
        return "";
    if (__CODE_SIZE[group] <= item)
        return "";
    return __HTTP_REASONS[code / 100 - 1][code % 100];
}

static void webs_session_reset(WEBS_SESSION* session)
{
    free(session->req);
    session->req = NULL;
    session->req_size = session->header_size = session->data_size = 0;
    session->state = WEBS_SESSION_STATE_IDLE;
}

static inline WEBS_SESSION* webs_create_session(WEBS* webs)
{
    HANDLE h;
    WEBS_SESSION* session;
    if (so_count(&webs->sessions) >= WEBS_MAX_SESSIONS)
    {
#if (WEBS_DEBUG_ERRORS)
        printf("WEBS: Too many sessions, rejecting connection\n");
#endif //WEBS_DEBUG_ERRORS
        return NULL;
    }
    h = so_allocate(&webs->sessions);
    session = so_get(&webs->sessions, h);
    if (session == NULL)
        return NULL;
    session->state = WEBS_SESSION_STATE_IDLE;
    session->req = NULL;
    session->req_size = session->header_size = session->data_size = 0;
    session->io = io_create(WEBS_IO_SIZE + sizeof(TCP_STACK));
    session->self = h;
    if (session->io == NULL)
    {
        so_free(&webs->sessions, h);
        return NULL;
    }
#if (WEBS_SESSION_TIMEOUT_S)
    session->timer = timer_create(session->self, HAL_WEBS);
#endif //WEBS_SESSION_TIMEOUT_S
    return session;
}

static void webs_destroy_session(WEBS* webs, WEBS_SESSION* session)
{
    free(session->req);
#if (WEBS_SESSION_TIMEOUT_S)
    timer_stop(session->timer, session->self, HAL_WEBS);
    timer_destroy(session->timer);
#endif //WEBS_SESSION_TIMEOUT_S
    io_destroy(session->io);
    so_free(&webs->sessions, session->self);
}

static inline void webs_open_session(WEBS* webs, HANDLE conn)
{
    WEBS_SESSION* session = webs_create_session(webs);
    if (session == NULL)
    {
        tcp_close(webs->tcpip, conn);
        return;
    }
    session->conn = conn;

#if (WEBS_DEBUG_SESSION)
    tcp_get_remote_addr(webs->tcpip, conn, &session->remote_addr);
    printf("WEBS: new session from ");
    ip_print(&session->remote_addr);
    printf("\n");
#endif //WEBS_DEBUG_SESSION

#if (WEBS_SESSION_TIMEOUT_S)
    timer_start_ms(session->timer, WEBS_SESSION_TIMEOUT_S * 1000);
#endif //WEBS_SESSION_TIMEOUT_S

    tcp_read(webs->tcpip, session->conn, session->io, WEBS_IO_SIZE);
}

static inline void webs_close_session(WEBS* webs, WEBS_SESSION* session)
{
#if (WEBS_DEBUG_SESSION)
    printf("WEBS: closed session from ");
    ip_print(&session->remote_addr);
    printf("\n");
#endif //WEBS_DEBUG_SESSION

    tcp_close(webs->tcpip, session->conn);
    webs_destroy_session(webs, session);
}

static WEBS_SESSION* webs_find_session(WEBS* webs, HANDLE conn)
{
    HANDLE h;
    WEBS_SESSION* session;
    for (h = so_first(&webs->sessions); h != INVALID_HANDLE; h = so_next(&webs->sessions, h))
    {
        session = so_get(&webs->sessions, h);
        if (conn == session->conn)
            return session;
    }
    return NULL;
}

static void webs_out_of_memory(WEBS* webs, WEBS_SESSION* session)
{
#if (WEBS_DEBUG_ERRORS)
    printf("WEBS: Out of memory\n");
#endif //WEBS_DEBUG_ERRORS
    webs_close_session(webs, session);
}

static void webs_tx(WEBS* webs, WEBS_SESSION* session)
{
    TCP_STACK* tcp_stack;
    tcp_stack = io_push(session->io, sizeof(TCP_STACK));
    session->io->data_size = WEBS_IO_SIZE;
    if (session->req_size - session->processed < WEBS_IO_SIZE)
    {
        tcp_stack->flags = 0;
        session->io->data_size = session->req_size - session->processed;
    }
    else
        tcp_stack->flags = TCP_PSH;
    memcpy(io_data(session->io), session->req + session->processed, session->io->data_size);
    tcp_write(webs->tcpip, session->conn, session->io);
}

static inline void webs_generate_params(WEBS_SESSION* session, unsigned int response_size)
{
    //header
    web_set_str_param(io_data(session->io), &session->io->data_size, "server", "RExOS");

    if (response_size)
    {
        web_set_int_param(io_data(session->io), &session->io->data_size, "content-length", response_size);
        web_set_str_param(io_data(session->io), &session->io->data_size, "content-type", "text/html");
    }
}

static void webs_send_response(WEBS* webs, WEBS_SESSION* session, WEB_RESPONSE code, char* data, unsigned int data_size)
{
    unsigned int header_size, status_line_size;

    status_line_size = HTTP_STATUS_LINE_SIZE + strlen(webs_get_response_text(code));
    webs_generate_params(session, data_size);
    header_size = status_line_size + session->io->data_size + 2;
    free(session->req);
    session->req = malloc(header_size + data_size);

    if (session->req == NULL)
    {
        webs_out_of_memory(webs, session);
        return;
    }

    //status line
    sprintf(session->req, "HTTP/%d.%d %d %s\r\n", session->version >> 4, session->version & 0xf, code, webs_get_response_text(code));

    //header, generated in io
    memcpy(session->req + status_line_size, io_data(session->io), session->io->data_size);
    sprintf(session->req + status_line_size + session->io->data_size, "\r\n");
    memcpy(session->req + header_size, data, data_size);
    session->req_size = header_size + data_size;
    session->processed = 0;
    session->state = WEBS_SESSION_STATE_TX;

#if (WEBS_DEBUG_REQUESTS)
    printf("WEBS: %d %s\n", code, webs_get_response_text(code));
#endif //WEBS_DEBUG_REQUESTS
#if (WEBS_DEBUG_FLOW)
    printf("WEBS TX:\n");
    web_print(session->req, session->req_size);
#endif //WEBS_DEBUG_FLOW

#if (WEBS_SESSION_TIMEOUT_S)
    timer_start_ms(session->timer, WEBS_SESSION_TIMEOUT_S * 1000);
#endif //WEBS_SESSION_TIMEOUT_S

    webs_tx(webs, session);
}

static char* webs_get_error_html(WEBS* webs, WEB_RESPONSE code)
{
    int i;
    WEBS_ERROR* err;
    for (i = 0; i < array_size(webs->errors); ++i)
    {
        err = array_at(webs->errors, i);
        if (err->code == code)
            return err->data;
    }
    return NULL;
}

static void webs_respond_error(WEBS* webs, WEBS_SESSION* session, WEB_RESPONSE code)
{
    char* html = webs_get_error_html(webs, code);
    if (html == NULL)
        html = webs->generic_error;
    if (html == NULL)
    {
        //no generic error set
        webs_destroy_session(webs, session);
        return;
    }
    webs_session_reset(session);
    webs_send_response(webs, session, code, html, strlen(html));
}

static inline void webs_open(WEBS* webs, uint16_t port, HANDLE tcpip, HANDLE process)
{
    if (webs->process != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    webs->tcpip = tcpip;
    webs->listener = tcp_listen(webs->tcpip, port);
    if (webs->listener == INVALID_HANDLE)
        return;
    webs->process = process;
}

static inline void webs_close(WEBS* webs, HANDLE process)
{
    HANDLE h;
    WEBS_SESSION* session;
    if (process != webs->process)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    if (webs->process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    for (h = so_first(&webs->sessions); h != INVALID_HANDLE; h = so_next(&webs->sessions, h))
    {
        session = so_get(&webs->sessions, h);
        webs_close_session(webs, session);
    }
    tcp_close_listen(webs->tcpip, webs->listener);
    webs->process = INVALID_HANDLE;
}

static inline void webs_user_read(WEBS* webs, WEBS_SESSION* session, IO* io)
{
    io->data_size = 0;
    if (io_get_free(io) < session->data_size)
    {
        error(ERROR_IO_BUFFER_TOO_SMALL);
        return;
    }
    memcpy(io_data(io), session->req + session->header_size, session->data_size);
    io->data_size = session->data_size;
}

static inline void webs_user_write(WEBS* webs, WEBS_SESSION* session, IO* io)
{
    WEBS_SESSION* cur_session;
    HANDLE h;
    WEB_RESPONSE code = *((WEB_RESPONSE*)io_stack(io));
    io_pop(io, sizeof(WEB_RESPONSE));

    webs->busy = false;
    //switch to next req (if any)
    for (h = so_first(&webs->sessions); h != INVALID_HANDLE; h = so_next(&webs->sessions, h))
    {
        cur_session = so_get(&webs->sessions, h);
        if (cur_session->state == WEBS_SESSION_STATE_PENDING)
        {
            webs->busy = true;
            cur_session->state = WEBS_SESSION_STATE_REQUEST;
            ipc_post_inline(webs->process, HAL_CMD(HAL_WEBS, (WEBS_GET + cur_session->method)), cur_session->self,
                            cur_session->node_handle, cur_session->req_size);
            break;
        }
    }

    webs_send_response(webs, session, code, io_data(io), io->data_size);
}

static inline void webs_create_node(WEBS* webs, HANDLE process, HANDLE parent, IO* io, unsigned int flags)
{
    *((HANDLE*)io_data(io)) = web_node_allocate(&webs->web_node, parent, io_data(io), flags);
    io->data_size = sizeof(HANDLE);

    io_complete(process, HAL_IO_CMD(HAL_WEBS, WEBS_CREATE_NODE), parent, io);
    error(ERROR_SYNC);
}

static inline void webs_destroy_node(WEBS* webs, HANDLE handle)
{
    web_node_free(&webs->web_node, handle);
}

static inline void webs_register_error(WEBS* webs, int code, char* html)
{
    WEBS_ERROR* err;
    if (code == WEB_GENERIC_ERROR)
    {
        if (webs->generic_error != NULL)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return;
        }
        webs->generic_error = html;
        return;
    }
    if (webs_get_error_html(webs, code))
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    if ((err = array_append(&webs->errors)) == NULL)
        return;
    err->code = code;
    err->data = html;
}

static inline void webs_unregister_error(WEBS* webs, int code)
{
    int i;
    WEBS_ERROR* err;
    if (code == WEB_GENERIC_ERROR)
    {
        if (webs->generic_error == NULL)
        {
            error(ERROR_NOT_CONFIGURED);
            return;
        }
        webs->generic_error = NULL;
        return;
    }
    for (i = 0; i < array_size(webs->errors); ++i)
    {
        err = array_at(webs->errors, i);
        if (err->code == code)
        {
            array_remove(&webs->errors, i);
            return;
        }
    }
    error(ERROR_NOT_CONFIGURED);
}

static inline void webs_get_param(WEBS* webs, WEBS_SESSION* session, IO* io)
{
    unsigned int size;
    char* param;

    param = web_get_str_param(session->req, session->header_size, io_data(io), &size);
    io->data_size = 0;
    if (param == NULL)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    if (io_get_free(io) < size + 1)
    {
        error(ERROR_IO_BUFFER_TOO_SMALL);
        return;
    }
    memcpy(io_data(io), param, size);
    ((uint8_t*)io_data(io))[size] = 0;
    io->data_size = size + 1;
    io_complete(webs->process, HAL_IO_CMD(HAL_WEBS, WEBS_GET_PARAM), session->self, io);
    error(ERROR_SYNC);
}

static inline void webs_set_param(WEBS* webs, WEBS_SESSION* session, IO* io)
{
    char* param;
    char* value;

    param = io_data(io);
    value = param + strlen(param) + 1;
    web_set_str_param(io_data(session->io), &session->io->data_size, param, value);
}

static inline void webs_get_url(WEBS* webs, WEBS_SESSION* session, IO* io)
{
    io->data_size = 0;
    if (io_get_free(io) < session->url_size + 1)
    {
        error(ERROR_IO_BUFFER_TOO_SMALL);
        return;
    }
    memcpy(io_data(io), session->url, session->url_size);
    ((uint8_t*)io_data(io))[session->url_size] = 0;
    io->data_size = session->url_size + 1;
    io_complete(webs->process, HAL_IO_CMD(HAL_WEBS, WEBS_GET_URL), session->self, io);
    error(ERROR_SYNC);
}

static inline void webs_session_request(WEBS* webs, IPC* ipc)
{
    WEBS_SESSION* session = so_get(&webs->sessions, ipc->param1);

    if (session == NULL)
    {
        error(ERROR_CONNECTION_CLOSED);
        return;
    }
#if (WEBS_SESSION_TIMEOUT_S)
    if (HAL_ITEM(ipc->cmd) == IPC_TIMEOUT)
    {
#if (WEBS_DEBUG_SESSION)
        printf("WEBS: session timeout\n");
#endif //WEBS_DEBUG_SESSION
        webs_close_session(webs, session);
    }
    else
#endif //WEBS_SESSION_TIMEOUT_S
        if (session->state != WEBS_SESSION_STATE_REQUEST)
        {
            error(ERROR_INVALID_STATE);
            return;
        }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        webs_user_read(webs, session, (IO*)ipc->param2);
        break;
    case IPC_WRITE:
        webs_user_write(webs, session, (IO*)ipc->param2);
        break;
    case WEBS_GET_PARAM:
        webs_get_param(webs, session, (IO*)ipc->param2);
        break;
    case WEBS_SET_PARAM:
        webs_set_param(webs, session, (IO*)ipc->param2);
        break;
    case WEBS_GET_URL:
        webs_get_url(webs, session, (IO*)ipc->param2);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void webs_request(WEBS* webs, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        webs_open(webs, ipc->param1, ipc->param2, ipc->process);
        break;
    case IPC_CLOSE:
        webs_close(webs, ipc->process);
        break;
    case WEBS_CREATE_NODE:
        webs_create_node(webs, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
        break;
    case WEBS_DESTROY_NODE:
        webs_destroy_node(webs, (HANDLE)ipc->param1);
        break;
    case WEBS_REGISTER_RESPONSE:
        webs_register_error(webs, (int)ipc->param1, (char*)ipc->param2);
        break;
    case WEBS_UNREGISTER_RESPONSE:
        webs_unregister_error(webs, (int)ipc->param1);
        break;
    default:
        webs_session_request(webs, ipc);
    }
}

static inline void webs_req_received(WEBS* webs, WEBS_SESSION* session)
{
    char* str;
    unsigned int pos, size;
    do {
        io_reset(session->io);
        //parse status line
        //<METHOD> <URL> HTTP/<VERSION>
        str = session->req;
        size = web_get_line_size(session->req, session->header_size) - 2;

        //method
        if ((pos = web_get_word(str, size, ' ')) == 0)
            break;
        if (!web_get_method(str, pos, &session->method))
            break;
        size -= pos + 1;
        str += pos + 1;

        //URL
        session->url = str;
        if ((pos = web_get_word(str, size, ' ')) == 0)
            break;
        session->url_size = pos;
        if (!web_url_to_relative(&session->url, &session->url_size))
            break;
        size -= pos + 1;
        str += pos + 1;

        //version
        if (!web_get_version(str, size, &session->version))
            break;
        if (session->version > HTTP_1_1)
        {
            session->version = HTTP_1_1;
            webs_respond_error(webs, session, WEB_RESPONSE_HTTP_VERSION_NOT_SUPPORTED);
            return;
        }

#if (WEBS_DEBUG_REQUESTS)
        printf("WEBS: %s ", __HTTP_METHODS[session->method]);
        web_print(session->url, session->url_size);
        printf("\n");
#endif //WEBS_DEBUG_REQUESTS

        //check url path and method
        session->node_handle = web_node_find_path(&webs->web_node, session->url, session->url_size);
        if (session->node_handle == INVALID_HANDLE)
        {
            webs_respond_error(webs, session, WEB_RESPONSE_NOT_FOUND);
            return;
        }
        if (!web_node_check_flag(&webs->web_node, session->node_handle, 1 << session->method))
        {
            webs_respond_error(webs, session, WEB_RESPONSE_METHOD_NOT_ALLOWED);
            return;
        }

        if (webs->busy)
            session->state = WEBS_SESSION_STATE_PENDING;
        else
        {
            webs->busy = true;
            session->state = WEBS_SESSION_STATE_REQUEST;
            ipc_post_inline(webs->process, HAL_CMD(HAL_WEBS, (WEBS_GET + session->method)), session->self, session->node_handle, session->req_size);
        }
        return;
    } while (false);
    webs_respond_error(webs, session, WEB_RESPONSE_BAD_REQUEST);
}

static inline void webs_session_rx(WEBS* webs, WEBS_SESSION* session, int size)
{
    if (size < 0)
    {
        //any error will cause connection termination
        webs_close_session(webs, session);
        return;
    }

    //we don't need TCP flags analyse
    io_pop(session->io, sizeof(TCP_STACK));

    if (session->req_size + session->io->data_size > WEBS_MAX_PAYLOAD)
    {
        webs_respond_error(webs, session, WEB_RESPONSE_PAYLOAD_TOO_LARGE);
        return;
    }

    switch (session->state)
    {
    case WEBS_SESSION_STATE_IDLE:
        session->req = malloc(size);
        session->state = WEBS_SESSION_STATE_RX;
        break;
    case WEBS_SESSION_STATE_RX:
        session->req = realloc(session->req, session->req_size + size);
        break;
    default:
#if (WEBS_DEBUG_ERRORS)
        printf("WEBS: Invalid session state on RX: %d\n", session->state);
#endif //WEBS_DEBUG_ERRORS
        webs_close_session(webs, session);
        return;
    }

    if (session->req == NULL)
    {
        webs_out_of_memory(webs, session);
        return;
    }

    memcpy(session->req + session->req_size, io_data(session->io), session->io->data_size);
    session->req_size += session->io->data_size;

    //Header ends with double CRLF
    if (session->header_size == 0)
    {
        session->header_size = web_get_header_size(session->req, session->req_size);
        //still no header received
        if (session->header_size == 0)
        {
            tcp_read(webs->tcpip, session->conn, session->io, WEBS_IO_SIZE);
            return;
        }
    }

    session->status_line_size = web_get_line_size(session->req, session->header_size);
    //Make sure all data received
    if (session->data_size == 0)
        session->data_size = web_get_int_param(session->req + session->status_line_size, session->header_size, "content-length");
    if (session->data_size + session->header_size < session->req_size)
    {
        tcp_read(webs->tcpip, session->conn, session->io, WEBS_IO_SIZE);
        return;
    }

#if (WEBS_DEBUG_FLOW)
    printf("WEBS RX:\n");
    web_print(session->req, session->req_size);
#endif //WEBS_DEBUG_FLOW

#if (WEBS_SESSION_TIMEOUT_S)
    timer_stop(session->timer, session->self, HAL_WEBS);
#endif //WEBS_SESSION_TIMEOUT_S

    webs_req_received(webs, session);
}

static inline void webs_session_tx_complete(WEBS* webs, WEBS_SESSION* session, int size)
{
    if (size < 0)
    {
        //any error will cause connection termination
        webs_close_session(webs, session);
        return;
    }
    session->processed += size;
    if (session->processed >= session->req_size)
    {
#if (WEBS_SESSION_TIMEOUT_S)
        webs_session_reset(session);
        tcp_read(webs->tcpip, session->conn, session->io, WEBS_IO_SIZE);
#else
        webs_close_session(webs, session);
#endif //WEBS_SESSION_TIMEOUT_S
    }
    else
        webs_tx(webs, session);
}

static inline void webs_tcp_request(WEBS* webs, IPC* ipc)
{
    WEBS_SESSION* session;
    if (HAL_ITEM(ipc->cmd) == IPC_OPEN)
        webs_open_session(webs, (HANDLE)ipc->param1);
    else
    {
        session = webs_find_session(webs, (HANDLE)ipc->param1);
        //session may be closed before request complete. In this case just ignore. IO will be destroyed by kernel automatically.
        if (session == NULL)
            return;

        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_CLOSE:
            webs_destroy_session(webs, session);
            break;
        case IPC_READ:
            webs_session_rx(webs, session, (int)ipc->param3);
            break;
        case IPC_WRITE:
            webs_session_tx_complete(webs, session, (int)ipc->param3);
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

void webs_main()
{
    IPC ipc;
    WEBS webs;
    webs_init(&webs);
#ifdef WEBS_DEBUG
    open_stdout();
#endif //WEBS_DEBUG
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_WEBS:
            webs_request(&webs, &ipc);
            break;
        case HAL_TCP:
            webs_tcp_request(&webs, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}

