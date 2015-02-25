/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CANVAS_H
#define CANVAS_H

#include "sys.h"
#include <stdint.h>

typedef struct {
    unsigned int size;
    unsigned short width, height;
    unsigned short bits_per_pixel;
} CANVAS;

#define CANVAS_DATA(canvas)                         ((uint8_t*)((unsigned int)(canvas) + sizeof(CANVAS)))

HANDLE canvas_create(unsigned short width, unsigned short height, unsigned short bits_per_pixel);
CANVAS* canvas_open(HANDLE block);
bool canvas_resize(CANVAS* canvas, unsigned short width, unsigned short height, unsigned short bits_per_pixel);
void canvas_clear(CANVAS* canvas);
void canvas_destroy(HANDLE block);

#endif // CANVAS_H
