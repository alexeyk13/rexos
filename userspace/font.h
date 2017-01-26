/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FONT_H
#define FONT_H

#include "canvas.h"
#include "graphics.h"

#define FONT_ALIGN_LEFT                 (0 << 0)
#define FONT_ALIGN_RIGHT                (1 << 0)
#define FONT_ALIGN_HCENTER              (2 << 0)
#define FONT_ALIGN_HMASK                (3 << 0)
#define FONT_ALIGN_TOP                  (0 << 2)
#define FONT_ALIGN_BOTTOM               (1 << 2)
#define FONT_ALIGN_VCENTER              (2 << 2)
#define FONT_ALIGN_VMASK                (3 << 2)
#define FONT_ALIGN_CENTER               (FONT_ALIGN_HCENTER | FONT_ALIGN_VCENTER)

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

unsigned short font_get_glyph_width(FACE* face, unsigned short num);
void font_render_glyph(CANVAS* canvas, const POINT* point, FACE* face, unsigned short height, unsigned short num);
unsigned short font_get_char_width(const FONT *font, const char* utf8);
void font_render_char(CANVAS* canvas, const POINT* point, const FONT *font, const char* utf8);
unsigned short font_get_text_width(const FONT *font, const char* utf8);
void font_render_text(CANVAS* canvas, const POINT *point, const FONT *font, const char* utf8);
void font_render(CANVAS* canvas, const RECT* rect, const FONT *font, const char* utf8, unsigned int align);

#endif // FONT_H
