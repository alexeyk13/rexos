/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GUI_H
#define GUI_H

#include "types.h"
#include "cc_macro.h"
#include <stdbool.h>

typedef struct {
    unsigned short x, y;
} POINT;

typedef struct {
    unsigned short left, top, width, height;
} RECT;

typedef struct {
    unsigned int size;
    unsigned short width, height;
    unsigned short bits_per_pixel;
} CANVAS;

typedef struct {
    HANDLE (*lib_canvas_create)(unsigned short, unsigned short, unsigned short);
    CANVAS* (*lib_canvas_open)(HANDLE);
    bool (*lib_canvas_resize)(CANVAS*, unsigned short, unsigned short, unsigned short);
    void (*lib_canvas_clear)(CANVAS*);
    void (*lib_canvas_destroy)(HANDLE block);
    void (*lib_graphics_put_pixel)(CANVAS*, POINT*, unsigned int);
    unsigned int (*lib_graphics_get_pixel)(CANVAS*, POINT*);
    void (*lib_graphics_clear_rect)(CANVAS*, RECT*);
    void (*lib_graphics_write_rect)(CANVAS*, RECT*, RECT*, const uint8_t*, unsigned int);
} LIB_GUI;

#define GUI_MODE_OR                         0x0
#define GUI_MODE_XOR                        0x1
#define GUI_MODE_AND                        0x2
#define GUI_MODE_FILL                       0x3

#endif // GUI_H
