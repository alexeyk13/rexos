/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include "io.h"

typedef enum {
    HTTP_0_9 = 0x09,
    HTTP_1_0 = 0x10,
    HTTP_1_1 = 0x11,
    HTTP_2_0 = 0x20,
} HTTP_VERSION;

typedef struct {
    char* s;
    unsigned int len;
} STR;

typedef struct {
    //remove me
    int method;
    HTTP_VERSION version;
    //remove me
    int resp;
    //remove me
    int content_type;
    STR url, head, body;
} HTTP;

void http_print(STR* str);
void http_set_error(HTTP* http, int response);
bool http_parse_request(IO* io, HTTP* http);
bool http_make_response(HTTP* http, IO* io);
bool http_find_param(HTTP* http, char* param, STR* value);
bool http_get_host(HTTP* http, STR* value);
bool http_get_path(HTTP* http, STR* path);
bool http_compare_path(STR* str1, char* str2);

#endif // HTTP_H
