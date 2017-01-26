/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "canvas.h"
#include <string.h>
#include "stdlib.h"

CANVAS* canvas_create(unsigned short width, unsigned short height, unsigned short bits_per_pixel)
{
    unsigned int data_size = ((width * height * bits_per_pixel + 7) >> 3) + 1;
    CANVAS* canvas = malloc(sizeof(CANVAS) + data_size);
    if (canvas == NULL)
        return NULL;
    canvas->size = data_size;
    canvas->width = width;
    canvas->height = height;
    canvas->bits_per_pixel = bits_per_pixel;
    canvas->data = ((void*)canvas) + sizeof(CANVAS);
    return canvas;
}

IO* canvas_create_io(unsigned short width, unsigned short height, unsigned short bits_per_pixel)
{
    unsigned int data_size = ((width * height * bits_per_pixel + 7) >> 3) + 1;
    IO* io = io_create(sizeof(CANVAS) + data_size);
    if (io == NULL)
        return NULL;
    CANVAS* canvas = io_data(io);
    canvas->size = data_size;
    canvas->width = width;
    canvas->height = height;
    canvas->bits_per_pixel = bits_per_pixel;
    canvas->data = io_data(io) + sizeof(CANVAS);
    return io;
}

bool canvas_resize(CANVAS* canvas, unsigned short width, unsigned short height, unsigned short bits_per_pixel)
{
    unsigned int new_size = ((width * height * bits_per_pixel + 7) >> 3) + 1;
    if (new_size > canvas->size)
    {
        error(ERROR_OUT_OF_RANGE);
        return false;
    }
    canvas->width = width;
    canvas->height = height;
    canvas->bits_per_pixel = bits_per_pixel;
    return true;
}

void canvas_clear(CANVAS* canvas)
{
    memset(canvas->data, 0, canvas->size);
}

void canvas_destroy(CANVAS* canvas)
{
    free(canvas);
}

void canvas_destroy_io(IO* io)
{
    io_destroy(io);
}
