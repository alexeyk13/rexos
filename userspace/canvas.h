/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CANVAS_H
#define CANVAS_H

#include "gui.h"
#include "lib.h"

#define CANVAS_DATA(canvas)                         ((uint8_t*)((unsigned int)(canvas) + sizeof(CANVAS)))

__STATIC_INLINE HANDLE canvas_create(unsigned short width, unsigned short height, unsigned short bits_per_pixel)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_canvas_create(width, height, bits_per_pixel);
}

__STATIC_INLINE CANVAS* canvas_open(HANDLE block)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_canvas_open(block);
}

__STATIC_INLINE bool canvas_resize(CANVAS* canvas, unsigned short width, unsigned short height, unsigned short bits_per_pixel)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_canvas_resize(canvas, width, height, bits_per_pixel);
}

__STATIC_INLINE void canvas_clear(CANVAS* canvas)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_canvas_clear(canvas);
}

__STATIC_INLINE void canvas_destroy(HANDLE block)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_canvas_destroy(block);
}

#endif // CANVAS_H
