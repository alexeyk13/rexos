/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "canvas.h"

#define GUI_MODE_OR                         0x0
#define GUI_MODE_XOR                        0x1
#define GUI_MODE_AND                        0x2
#define GUI_MODE_FILL                       0x3

typedef struct {
    unsigned short x, y;
} POINT;

typedef struct {
    unsigned short left, top, width, height;
} RECT;

void put_pixel(CANVAS* canvas, const POINT* point, unsigned int color);
unsigned int get_pixel(CANVAS* canvas, const POINT* point);
void line(CANVAS* canvas, const POINT* a, const POINT* b, unsigned int color);
void filled_rect(CANVAS* canvas, const RECT *rect, unsigned int color, unsigned int mode);
void image(CANVAS* canvas, const RECT* rect, const RECT* data_rect, const uint8_t* pix, unsigned int mode);

#endif // GRAPHICS_H
