/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "utf.h"

unsigned int utf8_char_len(const char* utf8)
{
    unsigned int size;
    uint8_t byte = (uint8_t)utf8[0];
    //one byte
    if (byte == 0x00)
        return 0;
    if (byte < 0x80)
        return 1;
    for (size = 0; byte & (1 << 7); byte <<= 1, ++size) {}
    return size;
}

uint32_t utf8_to_utf32(const char *utf8)
{
    unsigned int i, size;
    uint32_t res;
    size = utf8_char_len(utf8);
    if (size < 2)
        return (uint32_t)utf8[0];
    res = ((uint8_t)utf8[0]) & ((1 << (7 - size)) - 1);
    for (i = 1; i < size; ++i)
        res = (res << 6) | ((uint8_t)utf8[i] & 0x3f);
    return res;
}

unsigned int utf8_len(const char* utf8)
{
    unsigned int cnt = 0;
    unsigned int len = 0;
    while (utf8[len])
    {
        len += utf8_char_len(utf8 + len);
        ++cnt;
    }
    return cnt;
}

unsigned int utf16_len(const uint16_t* utf16)
{
    unsigned int i;
    for (i = 0;; ++i)
        if (utf16[i] == 0)
            return i + 1;
}

unsigned int utf16_to_latin1(const uint16_t* utf16, uint8_t* latin1)
{
    unsigned int i;
    for (i = 0;; ++i)
    {
        if (utf16[i] == 0x0000)
            break;
        if (utf16[i] > 0xff)
            latin1[i] = '?';
        else
            latin1[i] = (utf16[i] & 0xff);
    }
    latin1[i] = '\x0';
    return i;
}
