/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_FONT_H
#define LIB_FONT_H

#include "../userspace/gui.h"

unsigned short lib_font_get_glyph_width(FACE* face, unsigned short num);
void lib_font_render_glyph(CANVAS* canvas, POINT* point, FACE* face, unsigned short height, unsigned short num);
unsigned short lib_font_get_char_width(FONT* font, const char* utf8);
void lib_font_render_char(CANVAS* canvas, POINT* point, FONT* font, const char* utf8);
void lib_font_render_text(CANVAS* canvas, POINT* point, FONT* font, const char* utf8);

#endif // LIB_FONT_H
