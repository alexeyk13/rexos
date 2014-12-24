/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_UTF8_H
#define LIB_UTF8_H

#include <stdint.h>

unsigned int lib_utf8_char_len(const char* utf8);
uint32_t lib_utf8_to_utf32(const char* utf8);
unsigned int lib_utf8_len(const char* utf8);

#endif // LIB_UTF8_H
