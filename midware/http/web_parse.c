/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "web_parse.h"
#include "../../userspace/types.h"
#include "../../userspace/stdio.h"
#include <string.h>

const char* const __HTTP_URL_HEAD =       "http://";

const char* const __HTTP_VER =            "HTTP/";

const char* const __HTTP_METHODS[HTTP_METHODS_COUNT] =
                                         {"GET",
                                          "HEAD",
                                          "POST",
                                          "PUT",
                                          "DELETE",
                                          "CONNECT",
                                          "OPTIONS",
                                          "TRACE"};

unsigned int web_get_header_size(const char *data, unsigned int size)
{
    const char* cur;
    const char* next;
    if (size < 4)
        return 0;
    for (cur = data; (cur - data + 4 <= size) && (next = memchr(cur, '\r', size - (cur - data) - 3)) != NULL; cur = next + 1)
    {
        if (strncmp(next + 1, "\n\r\n", 3) == 0)
            return next - data + 4;
    }
    return 0;
}

int web_get_line_size(const char* data, unsigned int size)
{
    const char* cur;
    const char* next;
    if (size == 0)
        return -1;
    for (cur = data; (cur - data + 2 <= size) && (next = memchr(cur, '\r', size - (cur - data) - 1)) != NULL; cur = next + 1)
    {
        if (next[1] == '\n')
            return (next - data + 2);
    }
    return size;
}

unsigned int web_get_word(const char* data, unsigned int size, char delim)
{
    char* p;
    if (!size)
        return 0;
    p = memchr(data, delim, size);
    if (p == NULL)
        return size;
    return p - data;
}

int web_find_keyword(const char *data, unsigned int size, const char* const* keywords, unsigned int keywords_count)
{
    unsigned int i;
    for (i = 0; i < keywords_count; ++i)
    {
        if ((strlen(keywords[i]) == size) && (strncmp(data, keywords[i], size) == 0))
            return i;
    }
    return -1;
}

bool web_atou(const char* data, unsigned int size, unsigned int* u)
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

bool web_stricmp(const char* data, unsigned int size, const char* keyword)
{
    unsigned int i;
    char c, k;
    for (i = 0; i < size; ++i)
    {
        if (!keyword[i])
            return false;
        c = data[i];
        k = keyword[i];
        if (c >= 'A' && c <= 'Z')
            c += 0x20;
        if (k >= 'A' && k <= 'Z')
            k += 0x20;
        if (c != k)
            return false;
    }
    return true;
}

char* web_trim(char *str, unsigned int* len)
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


char* web_get_str_param(const char* head, unsigned int head_size, const char *param, unsigned int* value_len)
{
    int cur, cur_text;
    char* delim;
    for (; (cur = web_get_line_size(head, head_size)) > 0; head += cur, head_size -= cur)
    {
        cur_text = (cur >= head_size) ? cur : cur - 2;
        if ((delim = memchr(head, ':', cur_text)) != NULL)
        {
            if (web_stricmp(head, delim - head, param))
            {
                ++delim;
                *value_len = cur_text - (delim - head);
                return web_trim(delim, value_len);
            }
        }
    }
    return NULL;
}

unsigned int web_get_int_param(const char *head, unsigned int head_size, const char *param)
{
    char* cur_txt;
    unsigned int cur_len;
    unsigned int res = 0;
    cur_txt = web_get_str_param(head, head_size, param, &cur_len);
    if (cur_txt != NULL)
        web_atou(cur_txt, cur_len, &res);
    return res;
}

static unsigned int web_param_capitalize(char* head, const char* param)
{
    unsigned int i, len;
    bool nextUp = true;
    len = strlen(param);
    for (i = 0; i < len; ++i)
    {
        if (nextUp && param[i] >= 'a' && param[i] <= 'z')
            head[i] = param[i] - 0x20;
        else
            head[i] = param[i];
        nextUp = param[i] == '-';
    }
    return len;
}

void web_set_str_param(char* head, unsigned int* head_size, const char* param, const char* value)
{
    unsigned int tmp;
    //parameter already set. No override.
    if (web_get_str_param(head, *head_size, param, &tmp))
        return;
    *head_size += web_param_capitalize(head + *head_size, param);
    sprintf(head + *head_size, ": %s\r\n", value);
    *head_size += 4 + strlen(value);
}

void web_set_int_param(char* head, unsigned int* head_size, const char* param, int value)
{
    char buf[10];
    sprintf(buf, "%d", value);
    web_set_str_param(head, head_size, param, buf);
}

void web_print(char* data, unsigned int size)
{
    int start, cur, i, cur_line;
    for (start = 0; (cur = web_get_line_size(data + start, size - start)) >= 0; start += cur)
    {
        cur_line = (start + cur < size) ? cur - 2 : cur;
        for (i = 0; i < cur_line; ++i)
            putc(data[start + i]);
        if (cur_line != cur)
            putc('\n');
    }
}

bool web_url_to_relative(char** url, unsigned int* url_size)
{
    char* cur;
    //absolute?
    if ((*url)[0] != '/')
    {
        if (!web_stricmp((*url), (*url_size), __HTTP_URL_HEAD))
            return false;
        (*url_size) -= HTTP_URL_HEAD_LEN;
        (*url) += HTTP_URL_HEAD_LEN;
        //remove host
        if ((cur = memchr((*url), '/', (*url_size))) == NULL)
            return false;
        (*url_size) -= cur - (*url);
        (*url) = cur;
    }
    //remove tailing slashes
    while (((*url_size) > 1) && ((*url)[(*url_size) -1] == '/'))
        --(*url_size);
    return true;
}

bool web_get_method(char* data, unsigned int size, WEB_METHOD* method)
{
    int idx;
    if ((idx = web_find_keyword(data, size, __HTTP_METHODS, HTTP_METHODS_COUNT)) < 0)
        return false;
    *method = idx;
    return true;
}

bool web_get_version(const char* data, unsigned int size, HTTP_VERSION* version)
{
    char* dot;
    unsigned int ver_hi, ver_lo;
    if (strncmp(data, __HTTP_VER, HTTP_VER_LEN))
        return false;
    data += HTTP_VER_LEN;
    size -= HTTP_VER_LEN;
    if ((dot = memchr(data, '.', size)) == NULL)
        return false;
    if (!web_atou(data, dot - data, &ver_hi) || !web_atou(dot + 1, size - (dot - data) -1, &ver_lo))
        return false;
    *version = (ver_hi << 4) | ver_lo;
    return true;
}

