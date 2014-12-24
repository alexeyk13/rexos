/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FONT_H
#define FONT_H

#include "gui.h"
#include "lib.h"

__STATIC_INLINE unsigned short font_get_glyph_width(FACE* face, unsigned short num)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_get_glyph_width(face, num);
}

__STATIC_INLINE void font_render_glyph(CANVAS* canvas, POINT* point, FACE* face, unsigned short height, unsigned short num)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_render_glyph(canvas, point, face, height, num);
}

__STATIC_INLINE unsigned short font_get_char_width(FONT* font, const char* utf8)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_get_char_width(font, utf8);
}

__STATIC_INLINE void font_render_char(CANVAS* canvas, POINT* point, FONT* font, const char* utf8)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_render_char(canvas, point, font, utf8);
}

__STATIC_INLINE void font_render_text(CANVAS* canvas, POINT* point, FONT* font, const char* utf8)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_render_text(canvas, point, font, utf8);
}

#endif // FONT_H
