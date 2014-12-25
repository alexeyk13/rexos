/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FONT_H
#define FONT_H

#include "gui.h"
#include "lib.h"

#define FONT_ALIGN_LEFT                 (0 << 0)
#define FONT_ALIGN_RIGHT                (1 << 0)
#define FONT_ALIGN_HCENTER              (2 << 0)
#define FONT_ALIGN_HMASK                (3 << 0)
#define FONT_ALIGN_TOP                  (0 << 2)
#define FONT_ALIGN_BOTTOM               (1 << 2)
#define FONT_ALIGN_VCENTER              (2 << 2)
#define FONT_ALIGN_VMASK                (3 << 2)
#define FONT_ALIGN_CENTER               (FONT_ALIGN_HCENTER | FONT_ALIGN_VCENTER)

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

__STATIC_INLINE unsigned short font_get_text_width(FONT* font, const char* utf8)
{
    LIB_CHECK_RET(LIB_ID_GUI);
    return ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_get_text_width(font, utf8);
}

__STATIC_INLINE void font_render_text(CANVAS* canvas, POINT* point, FONT* font, const char* utf8)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_render_text(canvas, point, font, utf8);
}

__STATIC_INLINE void font_render(CANVAS* canvas, RECT* rect, FONT* font, const char* utf8, unsigned int align)
{
    LIB_CHECK(LIB_ID_GUI);
    ((const LIB_GUI*)__GLOBAL->lib[LIB_ID_GUI])->lib_font_render(canvas, rect, font, utf8, align);
}


#endif // FONT_H
