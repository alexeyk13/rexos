/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "http.h"
#include "types.h"
#include <string.h>

static const char* __HTTP_REASON100[] =     {"Continue",
                                             "Switching Protocols"};
static const char* __HTTP_REASON200[] =     {"OK",
                                             "Created",
                                             "Accepted",
                                             "Non-Authoritative Information",
                                             "No Content",
                                             "Reset Content",
                                             "Partial Content"};
static const char* __HTTP_REASON300[] =     {"Multiple Choices",
                                             "Moved Permanently",
                                             "Found",
                                             "See Other",
                                             "Not Modified",
                                             "Use Proxy",
                                             "",
                                             "Temporary Redirect"};
static const char* __HTTP_REASON400[] =     {"Bad Request",
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
static const char* __HTTP_REASON500[] =     {"Internal Server Error",
                                             "Not Implemented",
                                             "Bad Gateway",
                                             "Service Unavailable",
                                             "Gateway Timeout",
                                             "HTTP Version Not Supported"};

static const char** const __HTTP_REASONS[] = {__HTTP_REASON100, __HTTP_REASON200, __HTTP_REASON300, __HTTP_REASON400, __HTTP_REASON500};

static int http_line_size(char* data, unsigned int size)
{
    char* cur;
    char* next;
    if (size == 0)
        return -1;
    for (cur = data; (next = memchr(cur, '\r', size + cur - data - 1)) != NULL; cur = next + 1)
    {
        if (next[1] == '\n')
            return (next - data + 2);
    }
    return size ? size : -1;
}

static void http_print_line(char* data, unsigned int size)
{
    int i;
    for (i = 0; i < size; ++i)
        printd("%c", data[i]);
    printd("\n");
}

HTTP_RESPONSE http_parse_request(char* data, unsigned int size, HTTP_REQUEST* req)
{
    int req_line_size, cur;
    req_line_size = http_line_size(data, size);
    if (req_line_size < 0)
        return HTTP_RESPONSE_BAD_REQUEST;
    //TODO: parse request
    req->head = data + req_line_size;
    for (req->head_len = 0; (cur = http_line_size(req->head + req->head_len, size - req_line_size - req->head_len)) > 2; req->head_len += cur) {}
    //no CRLF
    if ((cur != 2) || (strncmp(req->head + req->head_len, "\r\n", 2)))
        return HTTP_RESPONSE_BAD_REQUEST;
    req->body_len = size - req_line_size - req->head_len - 2;
    if (req->body_len)
        req->body = req->head + req->head_len + 2;
    else
        req->body = NULL;
    req->head_len -= 2;

    printd("req line: ");
    http_print_line(data, req_line_size - 2);

    int i, sz;
    printd("header: \n");
    for (i = 0; (sz = http_line_size(req->head + i, req->head_len - i)) >= 0; i += sz)
    {
        http_print_line(req->head + i, i + sz < req->head_len ? sz - 2 : sz);
    }
    printd("header end\n");

    return 200;
}

void test_print(HTTP_RESPONSE resp)
{
    printd("TEST: %d, %s\n", resp, __HTTP_REASONS[(resp / 100) - 1][resp % 100]);
}
