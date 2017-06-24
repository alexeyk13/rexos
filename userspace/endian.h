/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ENDIAN_H
#define ENDIAN_H

#include <stdint.h>
#include "cc_macro.h"

#define HTONS(s) ( (((s) << 8)&0xFFFF) | (((s) >> 8)&0xFFFF))
#define HTONL(l) ( ((l)<<24) |((l)>>24) | (((l)>>8)&0x0000FF00) | (((l)<<8)&0x00FF0000) )

__STATIC_INLINE unsigned short be2short(const uint8_t* be)
{
    return ((unsigned short)be[0] << 8) | (unsigned short)be[1];
}

__STATIC_INLINE unsigned int be2int(const uint8_t* be)
{
    return ((unsigned int)be[0] << 24) | ((unsigned int)be[1] << 16) | ((unsigned int)be[2] << 8) | ((unsigned int)be[3] << 0);
}

__STATIC_INLINE void short2be(uint8_t* be, unsigned short value)
{
    be[0] = (uint8_t)((value >> 8) & 0xff);
    be[1] = (uint8_t)((value >> 0) & 0xff);
}

__STATIC_INLINE void int2be(uint8_t* be, unsigned int value)
{
    be[0] = (uint8_t)((value >> 24) & 0xff);
    be[1] = (uint8_t)((value >> 16) & 0xff);
    be[2] = (uint8_t)((value >> 8) & 0xff);
    be[3] = (uint8_t)((value >> 0) & 0xff);
}

#endif // ENDIAN_H
