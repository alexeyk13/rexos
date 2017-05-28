/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef WEB_PARSE_H
#define WEB_PARSE_H

#include <stdbool.h>
#include "../../userspace/web.h"

#define HTTP_URL_HEAD_LEN                       7
extern const char* const __HTTP_URL_HEAD;

#define HTTP_VER_LEN                            5
extern const char* const __HTTP_VER;

#define HTTP_METHODS_COUNT                      8
const char* const __HTTP_METHODS[HTTP_METHODS_COUNT];


typedef enum {
    HTTP_0_9 = 0x09,
    HTTP_1_0 = 0x10,
    HTTP_1_1 = 0x11,
    HTTP_2_0 = 0x20,
} HTTP_VERSION;

unsigned int web_get_header_size(const char* data, unsigned int size);
int web_get_line_size(const char *data, unsigned int size);
unsigned int web_get_word(const char *data, unsigned int size, char delim);
int web_find_keyword(const char* data, unsigned int size, const char* const* keywords, unsigned int keywords_count);
bool web_atou(const char* data, unsigned int size, unsigned int* u);
bool web_stricmp(const char* data, unsigned int size, const char* keyword);
char* web_trim(char* str, unsigned int* len);
char* web_get_str_param(const char* head, unsigned int head_size, const char* param, unsigned int* value_len);
unsigned int web_get_int_param(const char* head, unsigned int head_size, const char* param);
void web_set_str_param(char* head, unsigned int* head_size, const char* param, const char* value);
void web_set_int_param(char* head, unsigned int* head_size, const char* param, int value);
void web_print(char* data, unsigned int size);
bool web_url_to_relative(char** url, unsigned int* url_size);
bool web_get_method(char* data, unsigned int size, WEB_METHOD* method);
bool web_get_version(const char* data, unsigned int size, HTTP_VERSION* version);

#endif // WEB_PARSE_H
