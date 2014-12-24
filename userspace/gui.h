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
    uint32_t total_size;
    uint32_t char_offset;
    uint16_t count;
    uint16_t width;
} FACE;

typedef struct {
    uint16_t face_count;
    uint16_t height;
    //face 0, than other faces
} FONT;

typedef struct {
    //canvas
    HANDLE (*lib_canvas_create)(unsigned short, unsigned short, unsigned short);
    CANVAS* (*lib_canvas_open)(HANDLE);
    bool (*lib_canvas_resize)(CANVAS*, unsigned short, unsigned short, unsigned short);
    void (*lib_canvas_clear)(CANVAS*);
    void (*lib_canvas_destroy)(HANDLE block);
    //graphics
    void (*lib_graphics_put_pixel)(CANVAS*, POINT*, unsigned int);
    unsigned int (*lib_graphics_get_pixel)(CANVAS*, POINT*);
    void (*lib_graphics_clear_rect)(CANVAS*, RECT*);
    void (*lib_graphics_write_rect)(CANVAS*, RECT*, RECT*, const uint8_t*, unsigned int);
    //font
    unsigned short (*lib_font_get_glyph_width)(FACE*, unsigned short);
    void (*lib_font_render_glyph)(CANVAS*, POINT*, FACE*, unsigned short, unsigned short);
    void (*lib_font_render_char)(CANVAS*, POINT*, FONT*, const char*);
    //utf8
    unsigned int (*lib_utf8_char_len)(const char*);
    uint32_t (*lib_utf8_to_utf32)(const char*);
    unsigned int (*lib_utf8_len)(const char*);

} LIB_GUI;

#define GUI_MODE_OR                         0x0
#define GUI_MODE_XOR                        0x1
#define GUI_MODE_AND                        0x2
#define GUI_MODE_FILL                       0x3

#endif // GUI_H
