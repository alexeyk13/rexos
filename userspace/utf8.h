/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

unsigned int utf8_char_len(const char* utf8);
uint32_t utf8_to_utf32(const char* utf8);
unsigned int utf8_len(const char* utf8);

#endif // UTF8_H
