/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CANVAS_H
#define CANVAS_H

#include "io.h"
#include <stdint.h>

typedef struct {
    unsigned int size;
    void* data;
    unsigned short width, height;
    unsigned short bits_per_pixel;
} CANVAS;

CANVAS* canvas_create(unsigned short width, unsigned short height, unsigned short bits_per_pixel);
IO* canvas_create_io(unsigned short width, unsigned short height, unsigned short bits_per_pixel);
bool canvas_resize(CANVAS* canvas, unsigned short width, unsigned short height, unsigned short bits_per_pixel);
void canvas_clear(CANVAS* canvas);
void canvas_destroy(CANVAS* canvas);
void canvas_destroy_io(IO* io);

#endif // CANVAS_H
