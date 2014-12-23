/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_GRAPHICS_H
#define LIB_GRAPHICS_H

#include "../userspace/gui.h"

/*
    Graphics libraty. Only monochrome display is supported right now
 */

void lib_graphics_put_pixel(CANVAS* canvas, unsigned short x, unsigned short y, unsigned int color);
unsigned int lib_graphics_get_pixel(CANVAS* canvas, unsigned short x, unsigned short y);
void lib_graphics_clear_rect(CANVAS* canvas, RECT* rect);
void lib_graphics_write_rect(CANVAS* canvas, RECT* rect, uint8_t* data, RECT* data_rect, unsigned int mode);

#endif // LIB_GRAPHICS_H
