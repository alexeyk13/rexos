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

#define HTTP_LINE_SIZE                      64

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

static const char* __HTTP_URL_HEAD =        "http://";
#define HTTP_URL_HEAD_LEN                   7

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

static bool http_stricmp(STR* str, const char* keyword)
{
    unsigned int i;
    char c;
    for (i = 0; i < str->len; ++i)
    {
        c = str->s[i];
        if (c >= 'A' && c <= 'Z')
            c += 0x20;
        if (c != keyword[i])
            return false;
    }
    return true;
}

static void http_trim(STR* str)
{
    while (str->len && (str->s[0] == ' ' || str->s[0] == '\t'))
    {
        ++str->s;
        --str->len;
    }
    while (str->len && (str->s[str->len - 1] == ' ' || str->s[str->len - 1] == '\t'))
        --str->len;
}

void http_print(STR* str)
{
    int start, cur, i, cur_line;
    for (start = 0; (cur = http_line_size(str->s + start, str->len - start)) >= 0; start += cur)
    {
        cur_line = (start + cur < str->len) ? cur - 2 : cur;
        for (i = 0; i < cur_line; ++i)
            putc(str->s[start + i]);
        if (cur_line != cur)
            putc('\n');
    }
}

void http_set_error(HTTP* http, HTTP_RESPONSE response)
{
    http->head.s = http->body.s = NULL;
    http->head.len = http->body.len = 0;
    http->resp = response;
}

bool http_parse_request(IO* io, HTTP* http)
{
    int req_line_size, len, idx;
    unsigned int cur, ver_hi, ver_lo;
    req_line_size = http_line_size(io_data(io), io->data_size);
    http->resp = HTTP_RESPONSE_OK;

    do {
        if (req_line_size < 0)
            break;
        http->head.s = (char*)io_data(io) + req_line_size;
        for (http->head.len = 0; (cur = http_line_size(http->head.s + http->head.len, io->data_size - req_line_size - http->head.len)) > 2; http->head.len += cur) {}
        //no CRLF
        if ((cur != 2) || (strncmp(http->head.s + http->head.len, "\r\n", 2)))
            break;
        http->body.len = io->data_size - req_line_size - http->head.len - 2;
        if (http->body.len)
            http->body.s = http->head.s + http->head.len + 2;
        else
            http->body.s = NULL;
        http->head.len -= 2;
        req_line_size -= 2;

        cur = 0;
        //parse method
        if ((len = http_next_word(io_data(io), req_line_size, &cur)) < 3)
            break;
        if ((idx = http_find_keyword(io_data(io), len, __HTTP_METHOD, HTTP_METHOD_MAX)) < 0)
            break;
        http->method = idx;
        //parse URL
        cur += len;
        if ((len = http_next_word(io_data(io), req_line_size, &cur)) < 1)
            break;
        http->url.s = (char*)io_data(io) + cur;
        http->url.len = len;

        if ((http->url.s[0] != '/') && !(http->url.len > HTTP_URL_HEAD_LEN && http_stricmp(&http->url, __HTTP_URL_HEAD)))
            break;
        while ((http->url.len > 1) && http->url.s[http->url.len - 1] == '/')
            --http->url.len;
        //parse version
        cur += len;
        if ((len = http_next_word(io_data(io), req_line_size, &cur)) < HTTP_VER_LEN + 3)
            break;
        if (strncmp((char*)io_data(io) + cur, __HTTP_VER, HTTP_VER_LEN))
            break;
        cur += HTTP_VER_LEN;
        len -= HTTP_VER_LEN;
        if (!http_decode_float((char*)io_data(io) + cur, len, &ver_hi, &ver_lo))
            break;
        http->version = (ver_hi << 4) | ver_lo;
        if (http->version > HTTP_1_1)
        {
            http->version = HTTP_1_1;
            http->resp = HTTP_RESPONSE_HTTP_VERSION_NOT_SUPPORTED;
            return false;
        }

#if (HTTP_DEBUG)
        printf("HTTP: %s ", __HTTP_METHOD[http->method]);
        http_print(&http->url);
        printf("\n");
#if (HTTP_DEBUG_HEAD)
        http_print(&http->head);
        printf("\n");
#endif //HTTP_DEBUG_HEAD
#endif //HTTP_DEBUG
        return true;
    } while (false);
    http_set_error(http, HTTP_RESPONSE_BAD_REQUEST);
    return false;
}

static bool http_append_line(IO* io, char** head)
{
    unsigned int len = strlen(*head);
    if (io_get_free(io) < len + HTTP_LINE_SIZE)
        return false;
    io->data_size += len;
    (*head) += len;
    return true;
}

bool http_make_response(HTTP* http, IO* io)
{
    char* head = io_data(io);
    io->data_size = 0;
    if (io_get_free(io) < HTTP_LINE_SIZE)
        return false;
    //status line
    sprintf(head, "HTTP/%d.%d %d %s\r\n", http->version >> 4, http->version & 0xf, http->resp, __HTTP_REASONS[http->resp / 100 - 1][http->resp % 100]);
    if (!http_append_line(io, &head))
        return false;
    //header
    //TODO: user header
    sprintf(head, "Server: RExOS\r\n");
    if (!http_append_line(io, &head))
        return false;
    if (http->body.s != NULL && http->body.len)
    {
        sprintf(head, "Content-Length: %d\r\n", http->body.len);
        if (!http_append_line(io, &head))
            return false;
        //TODO: real content encode
        sprintf(head, "Content-Type: text/plain\r\n");
        if (!http_append_line(io, &head))
            return false;
    }
    //test!
    if (http->resp >= 400)
    {
        sprintf(head, "Connection: close\r\n");
        if (!http_append_line(io, &head))
            return false;
    }

    sprintf(head, "\r\n");
    if (!http_append_line(io, &head))
        return false;

    if (http->body.s != NULL && http->body.len)
    {
        if (io_get_free(io) < http->body.len)
            return false;
        memcpy(head, http->body.s, http->body.len);
        io->data_size += http->body.len;
    }

    ((char*)io_data(io))[io->data_size] = 0x00;
    printf("res: \n");
    printf("%s", io_data(io));
    printf("res end\n");
    return true;
}

bool http_find_param(HTTP* http, char* param, STR* value)
{
    STR str, k;
    int offset, cur;
    char* delim;
    unsigned int param_len = strlen(param);
    for (offset = 0; (cur = http_line_size(http->head.s + offset, http->head.len - offset)) > 2; offset += cur)
    {
        str.s = http->head.s + offset;
        str.len = (cur + offset < http->head.len) ? cur - 2 : cur;
        if ((delim = memchr(str.s, ':', str.len)) != NULL)
        {
            k.s = str.s;
            if (((k.len = delim - str.s) == param_len) && http_stricmp(&k, param))
            {
                value->s = delim + 1;
                value->len = str.len - k.len - 1;
                http_trim(value);
                return true;
            }
        }
    }
    return false;
}

bool http_get_host(HTTP* http, STR* value)
{
    char* delim;
    //search in params
    if (http_find_param(http, "host", value))
        return true;
    if (http->url.s[0] == '/')
        return false;
    value->s += HTTP_URL_HEAD_LEN;
    value->len -= HTTP_URL_HEAD_LEN;
    //search in url request
    if ((delim = memchr(value->s, '/', value->len)) != NULL)
        value->len = delim - value->s;
    return true;
}

bool http_get_path(HTTP* http, STR* path)
{
    char* delim;
    path->s = http->url.s;
    path->len = http->url.len;
    if (http->url.s[0] == '/')
        return true;
    path->s += HTTP_URL_HEAD_LEN;
    path->len -= HTTP_URL_HEAD_LEN;
    if ((delim = memchr(path->s, '/', path->len)) != NULL)
        path->len = path->len - (delim - path->s);
    else
    {
        http_set_error(http, HTTP_RESPONSE_BAD_REQUEST);
        return false;
    }
    return true;
}

bool http_compare_path(STR* str1, char* str2)
{
    if (strlen(str2) != str1->len)
        return false;
    return http_stricmp(str1, str2);
}
