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
#include "../../userspace/stdlib.h"
#include "../../userspace/sys.h"
#include "../../userspace/hs.h"
#include <string.h>
#include "sys_config.h"

void hss_main();

typedef enum {
    HSS_SESSION_STATE_IDLE,
    HSS_SESSION_STATE_RX,
    HSS_SESSION_STATE_TX,
    HSS_SESSION_STATE_TX_ERROR
} HSS_SESSION_STATE;

typedef enum {
    HTTP_0_9 = 0x09,
    HTTP_1_0 = 0x10,
    HTTP_1_1 = 0x11,
    HTTP_2_0 = 0x20,
} HTTP_VERSION;

typedef struct {
    IO* io;
    HANDLE conn;
#if (HS_DEBUG)
    IP remote_addr;
#endif //HS_DEBUG
    HSS_SESSION_STATE state;
    unsigned int content_size, processed, data_off;
    HTTP_CONTENT_TYPE content_type;
    HTTP_VERSION version;
    uint16_t method_idx;
} HSS_SESSION;

typedef struct {
    HANDLE tcpip, process, listener;
    bool busy;

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

#define HTTP_LINE_SIZE                         64

static const char* __HTTP_REASON100[] =        {"Continue",
                                               "Switching Protocols"};
static const char* __HTTP_REASON200[] =        {"OK",
                                               "Created",
                                               "Accepted",
                                               "Non-Authoritative Information",
                                               "No Content",
                                               "Reset Content",
                                               "Partial Content"};
static const char* __HTTP_REASON300[] =        {"Multiple Choices",
                                               "Moved Permanently",
                                               "Found",
                                               "See Other",
                                               "Not Modified",
                                               "Use Proxy",
                                               "",
                                               "Temporary Redirect"};
static const char* __HTTP_REASON400[] =        {"Bad Request",
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
static const char* __HTTP_REASON500[] =         {"Internal Server Error",
                                                "Not Implemented",
                                                "Bad Gateway",
                                                "Service Unavailable",
                                                "Gateway Timeout",
                                                "HTTP Version Not Supported"};

static const char** const __HTTP_REASONS[] =    {__HTTP_REASON100, __HTTP_REASON200, __HTTP_REASON300, __HTTP_REASON400, __HTTP_REASON500};


static const char* __HTTP_CONTENT[] =           {"text/plain",
                                                "text/html",
                                                "application/x-www-form-urlencoded"};

static const char* __HTTP_METHODS[] =           {"GET",
                                                "HEAD",
                                                "POST",
                                                "PUT",
                                                "DELETE",
                                                "CONNECT",
                                                "OPTIONS",
                                                "TRACE"};
#define HTTP_METHODS_COUNT                      8

static const char* __HTTP_VER =                 "HTTP/";
#define HTTP_VER_LEN                            5

static const char* __HTTP_URL_HEAD =            "http://";
#define HTTP_URL_HEAD_LEN                       7

static int hss_get_head_size(char* data, unsigned int size)
{
    char* cur;
    char* next;
    if (size < 4)
        return -1;
    for (cur = data; (cur - data + 4 <= size) && (next = memchr(cur, '\r', size - (cur - data) - 3)) != NULL; cur = next + 1)
    {
        if (strncmp(next + 1, "\n\r\n", 3) == 0)
            return next - data;
    }
    return -1;
}

static int hs_get_line_size(char* data, unsigned int size)
{
    char* cur;
    char* next;
    if (size == 0)
        return -1;
    for (cur = data; (cur - data + 2 <= size) && (next = memchr(cur, '\r', size - (cur - data) - 1)) != NULL; cur = next + 1)
    {
        if (next[1] == '\n')
            return (next - data + 2);
    }
    return size;
}

static unsigned int hs_get_word(char* data, unsigned int size)
{
    char* p;
    if (!size)
        return 0;
    p = memchr(data, ' ', size);
    if (p == NULL)
        return size;
    return p - data;
}

static int hs_find_keyword(char* data, unsigned int size, const char** keywords, unsigned int keywords_count)
{
    unsigned int i;
    for (i = 0; i < keywords_count; ++i)
    {
        if ((strlen(keywords[i]) == size) && (strncmp(data, keywords[i], size) == 0))
            return i;
    }
    return -1;
}

static bool hs_atou(char* data, unsigned int size, unsigned int* u)
{
    unsigned int i;
    *u = 0;
    if (size == 0)
        return false;
    for (i = 0; i < size; ++i)
    {
        if (data[i] > '9' || data[i] < '0')
        {
            *u = 0;
            return false;
        }
        (*u) = (*u) * 10 + data[i] - '0';
    }
    return true;
}

static bool hs_stricmp(char* data, unsigned int size, const char* keyword)
{
    unsigned int i;
    char c;
    for (i = 0; i < size; ++i)
    {
        if (!keyword[i])
            return false;
        c = data[i];
        if (c >= 'A' && c <= 'Z')
            c += 0x20;
        if (c != keyword[i])
            return false;
    }
    return true;
}

static void* hs_trim(char* str, unsigned int* len)
{
    while ((*len) && (str[0] == ' ' || str[0] == '\t'))
    {
        ++str;
        --(*len);
    }
    while ((*len) && (str[(*len) - 1] == ' ' || str[(*len) - 1] == '\t'))
        --(*len);
    return str;
}


static char* hs_find_param(char* head, unsigned int head_size, char* param, unsigned int* value_len)
{
    int cur, cur_text;
    char* delim;
    for (; (cur = hs_get_line_size(head, head_size)) > 0; head += cur, head_size -= cur)
    {
        cur_text = (cur >= head_size) ? cur : cur - 2;
        if ((delim = memchr(head, ':', cur_text)) != NULL)
        {
            if (hs_stricmp(head, delim - head, param))
            {
                ++delim;
                *value_len = cur_text - (delim - head);
                return hs_trim(delim, value_len);
            }
        }
    }
    return NULL;
}

#if (HS_DEBUG)
void hs_print(char* data, unsigned int size)
{
    int start, cur, i, cur_line;
    for (start = 0; (cur = hs_get_line_size(data + start, size - start)) >= 0; start += cur)
    {
        cur_line = (start + cur < size) ? cur - 2 : cur;
        for (i = 0; i < cur_line; ++i)
            putc(data[start + i]);
        if (cur_line != cur)
            putc('\n');
    }
}
#endif //HS_DEBUG

static inline void hss_init(HSS* hss)
{
    hss->process = INVALID_HANDLE;
    hss->busy = false;

    //TODO: Multiple session support
    hss->session.io = NULL;
    hss->session.conn = INVALID_HANDLE;
}

static inline HSS_SESSION* hss_create_session(HSS* hss)
{
    //TODO: Multiple session support
    if (hss->session.conn != INVALID_HANDLE)
    {
        error(ERROR_TOO_MANY_HANDLES);
#if (HS_DEBUG)
        printf("HS: Too many connections\n");
#endif //HS_DEBUG
        return NULL;
    }
    //TODO: allocation queue (tcpip style)
    hss->session.io = io_create(HS_IO_SIZE + sizeof(TCP_STACK) + sizeof(HS_STACK));
    if (hss->session.io == NULL)
        return NULL;
    io_push(hss->session.io, sizeof(HS_STACK));
    hss->session.state = HSS_SESSION_STATE_IDLE;
    return &hss->session;
}

static void hss_destroy_session(HSS* hss, HSS_SESSION* session)
{
    //TODO: allocation queue (tcpip style)
    io_destroy(session->io);
    session->io = NULL;
    //TODO: multiple session support
    session->conn = INVALID_HANDLE;
}

static void hss_destroy_connection(HSS* hss, HSS_SESSION* session)
{
    tcp_close(hss->tcpip, session->conn);
    hss_destroy_session(hss, session);
}

static char* hss_append_line(IO* io, char* head)
{
    unsigned int len = strlen(head);
    io->data_size += len;
    return head + len;
}

static void hss_fill_header(HSS_SESSION* session, HTTP_RESPONSE resp)
{
    io_reset(session->io);
    char* head = io_data(session->io);

    //status line
    sprintf(head, "HTTP/%d.%d %d %s\r\n", session->version >> 4, session->version & 0xf, resp, __HTTP_REASONS[resp / 100 - 1][resp % 100]);
    head = hss_append_line(session->io, head);

    //header
    sprintf(head, "Server: RExOS\r\n");
    head = hss_append_line(session->io, head);

    if (session->content_size)
    {
        sprintf(head, "Content-Length: %d\r\n", session->content_size);
        head = hss_append_line(session->io, head);
        if (session->content_type < HTTP_CONTENT_NONE)
        {
            sprintf(head, "Content-Type: %s\r\n", __HTTP_CONTENT[session->content_type]);
            head = hss_append_line(session->io, head);
        }
    }

    sprintf(head, "\r\n");
    hss_append_line(session->io, head);
#if (HS_DEBUG)
    printf("HS: %d %s\n", resp, __HTTP_REASONS[resp / 100 - 1][resp % 100]);
#if (HS_DEBUG_HEAD)
    hs_print(io_data(session->io), session->io->data_size);
    printf("\n");
#endif //HS_DEBUG_HEAD
#endif //HS_DEBUG
}

static void hss_respond_error(HSS* hss, HSS_SESSION* session, HTTP_RESPONSE code)
{
    hss_fill_header(session, code);
    //TODO: TX
    hss_destroy_connection(hss, session);
}

static inline void hss_open(HSS* hss, uint16_t port, HANDLE tcpip, HANDLE process)
{
    if (hss->process != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    hss->tcpip = tcpip;
    hss->listener = tcp_listen(hss->tcpip, port);
    if (hss->listener == INVALID_HANDLE)
        return;
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
    hss_destroy_connection(hss, &hss->session);
    tcp_close_listen(hss->tcpip, hss->listener);
    hss->process = INVALID_HANDLE;
}

static inline void hss_response(HSS* hss, HSS_SESSION* session, int size)
{
    TCP_STACK* tcp_stack;
    HS_RESPONSE* resp;
    //TODO: multiple session support
    //session closed, ignore
    if (session->conn == INVALID_HANDLE)
        return;
    //user responded with error, close session
    if ((size < 0) || (session->state != HSS_SESSION_STATE_RX))
    {
        hss_destroy_connection(hss, session);
        return;
    }
    if (session->processed < session->content_size)
    {
        //read more from tcp
        session->data_off = 0;
        io_reset(session->io);
        tcp_read(hss->tcpip, session->conn, session->io, HS_IO_SIZE);
        return;
    }
    resp = io_stack(session->io);
    if (resp->response != HTTP_RESPONSE_OK)
    {
        hss_respond_error(hss, session, resp->response);
        return;
    }
    session->state = HSS_SESSION_STATE_TX;
    session->processed = 0;
    session->content_size = resp->content_size;
    session->content_type = resp->content_type;
    hss_fill_header(session, resp->response);
    tcp_stack = io_push(session->io, sizeof(TCP_STACK));
    tcp_stack->flags = 0;
    tcp_write_sync(hss->tcpip, session->conn, session->io);
}

static inline void hss_write(HSS* hss, HSS_SESSION* session, IO* io)
{
    TCP_STACK* tcp_stack;
    //session closed, ignore
    if (session->conn == INVALID_HANDLE)
        return;
    //wrong state, close session
    if (session->state != HSS_SESSION_STATE_TX)
    {
        hss_destroy_connection(hss, session);
        error(ERROR_INVALID_STATE);
        return;
    }
    tcp_stack = io_push(io, sizeof(TCP_STACK));
    session->processed += io->data_size;
    tcp_stack->flags = (session->processed >= session->content_size) ? TCP_PSH : 0;

    if (tcp_write_sync(hss->tcpip, session->conn, io) < 0)
        return;

    if (session->processed >= session->content_size)
    {
        session->state = HSS_SESSION_STATE_IDLE;
        io_reset(session->io);
        tcp_read(hss->tcpip, session->conn, session->io, HS_IO_SIZE);
    }
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
    case HTTP_GET:
    case HTTP_HEAD:
    case HTTP_POST:
    case HTTP_PUT:
    case HTTP_DELETE:
    case HTTP_CONNECT:
    case HTTP_OPTIONS:
    case HTTP_TRACE:
        hss_response(hss, (HSS_SESSION*)ipc->param1, ipc->param3);
        break;
    case IPC_WRITE:
        hss_write(hss, (HSS_SESSION*)ipc->param1, (IO*)ipc->param2);
        break;
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

    hss_destroy_session(hss, session);
}

static inline bool hs_decode_version(char* data, unsigned int size, HSS_SESSION* session)
{
    char* dot;
    unsigned int ver_hi, ver_lo;
    if (strncmp(data, __HTTP_VER, HTTP_VER_LEN))
        return false;
    data += HTTP_VER_LEN;
    size -= HTTP_VER_LEN;
    if ((dot = memchr(data, '.', size)) == NULL)
        return false;
    if (!hs_atou(data, dot - data, &ver_hi) || !hs_atou(dot + 1, size - (dot - data) -1, &ver_lo))
        return false;
    session->version = (ver_hi << 4) | ver_lo;
    return true;
}

static inline bool hss_session_rx_idle(HSS* hss, HSS_SESSION* session)
{
    char* head;
    char* cur_txt;
    char* url;
    int icur, method_idx, url_size;
    unsigned int cur;
    int head_size = hss_get_head_size(io_data(session->io), session->io->data_size);
    if (head_size < 0)
    {
        //some stupid browsers, like konqueror, split request on few pcs with PSH flag
        if (io_get_free(session->io) > sizeof(HS_STACK) + sizeof(TCP_STACK))
        {
            session->io->data_offset += session->io->data_size;
            session->io->data_size = 0;
            tcp_read(hss->tcpip, session->conn, session->io, io_get_free(session->io) - sizeof(HS_STACK) - sizeof(TCP_STACK));
        }
        else
            hss_respond_error(hss, session, HTTP_RESPONSE_PAYLOAD_TOO_LARGE);
        return false;
    }
    cur_txt = io_data(session->io);
    //hide head
    session->io->data_offset += head_size + 4;
    session->io->data_size -= head_size + 4;
    session->data_off = head_size + 4;
    icur = hs_get_line_size(cur_txt, head_size);
    head = cur_txt + icur;
    head_size -= icur;
    icur -= 2;

    do {
        //decode status line
        //1. method
        if ((cur = hs_get_word(cur_txt, icur)) == 0)
            break;
        if ((method_idx = hs_find_keyword(cur_txt, cur, __HTTP_METHODS, HTTP_METHODS_COUNT)) < 0)
            break;
        cur_txt += cur + 1;
        icur -= cur + 1;
        session->method_idx = method_idx;

        //2. URL
        if ((url_size = hs_get_word(cur_txt, icur)) == 0)
            break;
        url = cur_txt;
        cur_txt += url_size + 1;
        icur -= url_size + 1;
        if ((url[0] != '/') && !hs_stricmp(url, url_size, __HTTP_URL_HEAD))
            break;

        //3. version
        if ((cur = hs_get_word(cur_txt, icur)) == 0)
            break;
        if (!hs_decode_version(cur_txt, cur, session))
            break;
        if (session->version > HTTP_1_1)
        {
            session->version = HTTP_1_1;
            hss_respond_error(hss, session, HTTP_RESPONSE_HTTP_VERSION_NOT_SUPPORTED);
            return false;
        }

        //4. find content size
        session->content_size = session->processed = 0;
        if ((cur_txt = hs_find_param(head, head_size, "content-length", &cur)) != NULL)
            hs_atou(cur_txt, cur, &session->content_size);
        if (session->content_size == 0)
            session->content_size = session->io->data_size;
        if (session->content_size)
        {
            session->content_type = HTTP_CONTENT_UNKNOWN;
            if ((cur_txt = hs_find_param(head, head_size, "content-type", &cur)) != NULL)
            {
                if ((icur = hs_find_keyword(cur_txt, cur, __HTTP_CONTENT, HTTP_CONTENT_NONE)) >= 0)
                    session->content_type = icur;
            }
        }
        else
            session->content_type = HTTP_CONTENT_NONE;

#if (HS_DEBUG)
        printf("HS: %s ", __HTTP_METHODS[method_idx]);
        hs_print(url, url_size);
        printf("\n");
#if (HS_DEBUG_HEAD)
        hs_print(head, head_size);
        printf("\n");
#endif //HS_DEBUG_HEAD
#endif //HS_DEBUG
        //TODO: check url
/*

        while ((http->url.len > 1) && http->url.s[http->url.len - 1] == '/')
            --http->url.len;
*/
        session->state = HSS_SESSION_STATE_RX;
        return true;
    } while (false);
    hss_respond_error(hss, session, HTTP_RESPONSE_BAD_REQUEST);
    return false;
}

static inline void hss_session_rx(HSS* hss, HSS_SESSION* session, int size)
{
    HS_STACK* stack;
    if (size < 0)
        //any error will cause connection termination
        return;

    //we don't need TCP flags analyse
    io_pop(session->io, sizeof(TCP_STACK));
    //unhide header before processing
    io_unhide(session->io);

    if (session->state == HSS_SESSION_STATE_IDLE)
    {
        if (!hss_session_rx_idle(hss, session))
            return;
    }

    //can receive more?
    session->processed += session->io->data_size;
    if ((session->processed < session->content_size) && (io_get_free(session->io) > sizeof(HS_STACK) + sizeof(TCP_STACK)))
    {
        //drop header if no content received
        if (session->processed == 0)
        {
            session->data_off = 0;
            io_reset(session->io);
        }
        else
        {
            session->io->data_offset += session->io->data_size;
            session->io->data_size = 0;
        }
        tcp_read(hss->tcpip, session->conn, session->io, io_get_free(session->io) - sizeof(HS_STACK) - sizeof(TCP_STACK));
        return;
    }

    io_unhide(session->io);
    session->io->data_offset += session->data_off;
    session->io->data_size -= session->data_off;
    stack = io_push(session->io, sizeof(HS_STACK));
    stack->content_size = session->content_size;
    stack->content_type = session->content_type;
    //TODO: object
    stack->obj = 0;

    //TODO: one request per time
    io_write(hss->process, HAL_IO_REQ(HAL_HTTP, (IPC_USER + session->method_idx)), (unsigned int)session, session->io);
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
        if (session->conn == INVALID_HANDLE)
            return;

        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_CLOSE:
            hss_close_session(hss, session);
            break;
        case IPC_READ:
            hss_session_rx(hss, session, (int)ipc->param3);
            break;
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

