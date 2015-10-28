/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "http.h"
#include "types.h"
#include <string.h>
#include "stdio.h"
#include "sys_config.h"

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

static const char* __HTTP_METHOD[] =        {"GET",
                                             "HEAD",
                                             "POST",
                                             "PUT",
                                             "DELETE",
                                             "CONNECT",
                                             "OPTIONS",
                                             "TRACE"};

static const char* __HTTP_VER =             "HTTP/";
#define HTTP_VER_LEN                        5

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

static int http_next_word(char* data, unsigned int size, unsigned int* from)
{
    int len;
    for (; ((*from) < size) && (data[*from] == ' ' || data[*from] == '\t'); ++(*from)) {}
    if (*from >= size)
        return -1;
    for (len = 0; (((*from) + len) < size) && (data[(*from) + len] != ' ') && (data[(*from) + len] != ' '); ++len) {}
    return len;
}

static int http_find_keyword(char* data, unsigned int size, const char** keywords, unsigned int keywords_count)
{
    unsigned int i;
    for (i = 0; i < keywords_count; ++i)
    {
        if ((strlen(keywords[i]) == size) && (strncmp(data, keywords[i], size) == 0))
            return i;
    }
    return -1;
}

static bool http_decode_float(char* data, unsigned int size, unsigned int* n, unsigned int* r)
{
    unsigned int i;
    bool r_mode = false;
    (*n) = (*r) = 0;
    for (i = 0; i < size; ++i)
    {
        //switch is generally faster in this context, than several if
        switch (data[i])
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (r_mode)
                (*r) = (*r) * 10 + data[i] - '0';
            else
                (*n) = (*n) * 10 + data[i] - '0';
            break;
        case '.':
            if (!r_mode)
            {
                r_mode = true;
                break;
            }
            //follow down
        default:
            return false;
        }
    }
    return true;
}

void http_print(char* data, unsigned int size)
{
    int start, cur, i, cur_line;
    for (start = 0; (cur = http_line_size(data + start, size - start)) >= 0; start += cur)
    {
        cur_line = (start + cur < size) ? cur - 2 : cur;
        for (i = 0; i < cur_line; ++i)
            putc(data[start + i]);
        if (cur_line != cur)
            putc('\n');
    }
}

HTTP_RESPONSE http_parse_request(char* data, unsigned int size, HTTP_REQUEST* req)
{
    int req_line_size, len, idx;
    unsigned int cur, ver_hi, ver_lo;
    req_line_size = http_line_size(data, size);
    if (req_line_size < 0)
        return HTTP_RESPONSE_BAD_REQUEST;
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
    req_line_size -= 2;

    cur = 0;
    //parse method
    if ((len = http_next_word(data, req_line_size, &cur)) < 3)
        return HTTP_RESPONSE_BAD_REQUEST;
    if ((idx = http_find_keyword(data, len, __HTTP_METHOD, HTTP_METHOD_MAX)) < 0)
        return HTTP_RESPONSE_BAD_REQUEST;
    req->method = idx;
    //parse URL
    cur += len;
    if ((len = http_next_word(data, req_line_size, &cur)) < 1)
        return HTTP_RESPONSE_BAD_REQUEST;
    req->url = data + cur;
    req->url_len = len;
    //parse version
    cur += len;
    if ((len = http_next_word(data, req_line_size, &cur)) < HTTP_VER_LEN + 3)
        return HTTP_RESPONSE_BAD_REQUEST;
    if (strncmp(data + cur, __HTTP_VER, HTTP_VER_LEN))
        return HTTP_RESPONSE_BAD_REQUEST;
    cur += HTTP_VER_LEN;
    len -= HTTP_VER_LEN;
    if (!http_decode_float(data + cur, len, &ver_hi, &ver_lo))
        return HTTP_RESPONSE_BAD_REQUEST;
    req->version = (ver_hi << 4) | ver_lo;
    if (req->version > HTTP_1_1)
    {
        req->version = HTTP_1_1;
        return HTTP_RESPONSE_HTTP_VERSION_NOT_SUPPORTED;
    }

#if (HTTP_DEBUG)
    printf("HTTP: %s ", __HTTP_METHOD[req->method]);
    http_print(req->url, req->url_len);
    printf("\n");
#if (HTTP_DEBUG_HEAD)
    http_print(req->head, req->head_len);
    printf("\n");
#endif //HTTP_DEBUG_HEAD
#endif //HTTP_DEBUG

    return HTTP_RESPONSE_OK;
}

void test_print(HTTP_RESPONSE resp)
{
    printd("TEST: %d, %s\n", resp, __HTTP_REASONS[(resp / 100) - 1][resp % 100]);
}
