/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "conv.h"
#include <string.h>

static int hex_decode_char(char c)
{
    //switch is generally faster, than several if on ARM
    switch (c)
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
        return c - '0';
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return c - 'A' + 0xa;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        return c - 'a' + 0xa;
    default:
        return -1;
    }
}

static char hex_encode_char(uint8_t val)
{
    if (val < 0xa)
        return val + '0';
    return val - 0xa + 'A';
}

int hex_decode(char* text, uint8_t* data, unsigned int size_max)
{
    int i, tmp;
    unsigned int cnt = strlen(text);
    if (cnt & 1)
        return -1;
    cnt = cnt >> 1;
    if (cnt > size_max)
        cnt = size_max;
    for (i = 0; i < cnt; ++i)
    {
        tmp = hex_decode_char(text[i << 1]);
        if (tmp < 0)
            return -1;
        data[i] = tmp << 4;
        tmp = hex_decode_char(text[(i << 1) + 1]);
        if (tmp < 0)
            return -1;
        data[i] |= tmp;
    }
    return cnt;
}

void hex_encode(uint8_t* data, unsigned int size, char* text)
{
    int i;
    for (i = 0; i < size; ++i)
    {
        text[i << 1] = hex_encode_char(data[i] >> 4);
        text[(i << 1) + 1] = hex_encode_char(data[i] & 0xf);
    }
    text[size << 1] = 0x00;
}
