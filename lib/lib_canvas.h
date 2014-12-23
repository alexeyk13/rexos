/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_CANVAS_H
#define LIB_CANVAS_H

#include "../userspace/gui.h"

HANDLE lib_canvas_create(unsigned short width, unsigned short height, unsigned short bits_per_pixel);
CANVAS* lib_canvas_open(HANDLE block);
bool lib_canvas_resize(CANVAS* canvas, unsigned short width, unsigned short height, unsigned short bits_per_pixel);
void lib_canvas_clear(CANVAS* canvas);
void lib_canvas_destroy(HANDLE block);

#endif // LIB_CANVAS_H
