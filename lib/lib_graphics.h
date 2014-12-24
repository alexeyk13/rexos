/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_GRAPHICS_H
#define LIB_GRAPHICS_H

#include "../userspace/gui.h"

void lib_graphics_put_pixel(CANVAS* canvas, POINT* point, unsigned int color);
unsigned int lib_graphics_get_pixel(CANVAS* canvas, POINT* point);
void lib_graphics_clear_rect(CANVAS* canvas, RECT* rect);
void lib_graphics_write_rect(CANVAS* canvas, RECT* rect, RECT* data_rect, const uint8_t* pix, unsigned int mode);

#endif // LIB_GRAPHICS_H
