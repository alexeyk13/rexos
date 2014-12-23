/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_canvas.h"
#include "../userspace/block.h"
#include "../userspace/canvas.h"
#include <string.h>

HANDLE lib_canvas_create(unsigned short width, unsigned short height, unsigned short bits_per_pixel)
{
    CANVAS* canvas = NULL;
    unsigned int data_size = ((width * height * bits_per_pixel + 7) >> 3) + 1;
    HANDLE block = block_create(sizeof(CANVAS) + data_size);
    if (block == INVALID_HANDLE)
        return INVALID_HANDLE;
    canvas = (CANVAS*)block_open(block);
    if (canvas == NULL)
        return INVALID_HANDLE;
    canvas->size = data_size;
    canvas->width = width;
    canvas->height = height;
    canvas->bits_per_pixel = bits_per_pixel;
    return block;
}

CANVAS* lib_canvas_open(HANDLE block)
{
    return (CANVAS*)block_open(block);
}

bool lib_canvas_resize(CANVAS* canvas, unsigned short width, unsigned short height, unsigned short bits_per_pixel)
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

void lib_canvas_clear(CANVAS* canvas)
{
    memset(CANVAS_DATA(canvas), 0, canvas->size);
}

void lib_canvas_destroy(HANDLE block)
{
    block_destroy(block);
}
