/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UTF_H
#define UTF_H

#include <stdint.h>

unsigned int utf8_char_len(const char* utf8);
uint32_t utf8_to_utf32(const char* utf8);
unsigned int utf8_len(const char* utf8);

unsigned int utf16_len(const uint16_t* utf16);
unsigned int utf16_to_latin1(const uint16_t* utf16, uint8_t* latin1);

#endif // UTF_H
