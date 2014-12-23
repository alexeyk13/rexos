/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "gui.h"
#include "lib.h"

__STATIC_INLINE void put_pixel(CANVAS* canvas, unsigned short x, unsigned short y, unsigned int color)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_graphics_put_pixel(canvas, x, y, color);
}

__STATIC_INLINE unsigned int get_pixel(CANVAS* canvas, unsigned short x, unsigned short y)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_graphics_get_pixel(canvas, x, y);
}

__STATIC_INLINE void clear_rect(CANVAS* canvas, RECT* rect)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_graphics_clear_rect(canvas, rect);
}

#endif // GRAPHICS_H
