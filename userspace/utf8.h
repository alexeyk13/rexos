/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UTF8_H
#define UTF8_H

#include "gui.h"
#include "lib.h"
#include <stdint.h>

__STATIC_INLINE uint32_t utf8_char_len(const char* utf8)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_utf8_char_len(utf8);
}

__STATIC_INLINE uint32_t utf8_to_utf32(const char* utf8)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_utf8_to_utf32(utf8);
}

__STATIC_INLINE uint32_t utf8_len(const char* utf8)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_utf8_len(utf8);
}

#endif // UTF8_H
