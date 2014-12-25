/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_font.h"
#include "lib_graphics.h"
#include "lib_utf8.h"
#include "../userspace/error.h"
#include "../userspace/heap.h"
#include "../userspace/font.h"

#define FACE_OFFSET(face, num)              (*((uint16_t*)((unsigned int)(face) + sizeof(FACE) + ((num) << 1))))
#define FACE_DATA(face)                     ((uint8_t*)((unsigned int)(face) + sizeof(FACE) + (((face)->count  + 1) << 1)))

unsigned short lib_font_get_glyph_width(FACE* face, unsigned short num)
{
    if (num >= face->count)
    {
        error(ERROR_OUT_OF_RANGE);
        return 0;
    }
    return FACE_OFFSET(face, num + 1) - FACE_OFFSET(face, num);
}

void lib_font_render_glyph(CANVAS* canvas, POINT* point, FACE* face, unsigned short height, unsigned short num)
{
    RECT rect, data_rect;
    if (num >= face->count)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    rect.left = point->x;
    rect.top = point->y;
    rect.width = FACE_OFFSET(face, num + 1) - FACE_OFFSET(face, num);
    rect.height = height;

    data_rect.top = 0;
    data_rect.height = height;
    data_rect.left = FACE_OFFSET(face, num);
    data_rect.width = face->width;
    lib_graphics_write_rect(canvas, &rect, &data_rect, FACE_DATA(face), GUI_MODE_FILL);
}

unsigned short lib_font_get_char_width(FONT* font, const char* utf8)
{
    int i;
    FACE* face;
    //convert to utf32
    uint32_t utf32 = lib_utf8_to_utf32(utf8);
    //find face in font
    face = (FACE*)((unsigned int)font + sizeof(FONT));
    for (i = 0; i < font->face_count; ++i)
    {
        if (face->char_offset <= utf32 && face->char_offset + face->count > utf32)
            return lib_font_get_glyph_width(face, utf32 - face->char_offset);
        face = (FACE*)((unsigned int)face + face->total_size);
    }
    //render nil symbol
    face = (FACE*)((unsigned int)font + sizeof(FONT));
    return lib_font_get_glyph_width(face, 0);
}

void lib_font_render_char(CANVAS* canvas, POINT* point, FONT* font, const char* utf8)
{
    int i;
    FACE* face;
    //convert to utf32
    uint32_t utf32 = lib_utf8_to_utf32(utf8);
    //find face in font
    face = (FACE*)((unsigned int)font + sizeof(FONT));
    for (i = 0; i < font->face_count; ++i)
    {
        if (face->char_offset <= utf32 && face->char_offset + face->count > utf32)
        {
            lib_font_render_glyph(canvas, point, face, font->height, utf32 - face->char_offset);
            return;
        }
        face = (FACE*)((unsigned int)face + face->total_size);
    }
    //render nil symbol
    face = (FACE*)((unsigned int)font + sizeof(FONT));
    lib_font_render_glyph(canvas, point, face, font->height, 0);
}

unsigned short lib_font_get_text_width(FONT* font, const char* utf8)
{
    unsigned int cur = 0;
    unsigned short len = 0;
    while (utf8[cur])
    {
        len += lib_font_get_char_width(font, utf8 + cur);
        cur += lib_utf8_char_len(utf8 + cur);
    }
    return len;
}

void lib_font_render_text(CANVAS* canvas, POINT* point, FONT* font, const char* utf8)
{
    unsigned int cur = 0;
    POINT cur_point;
    cur_point.x = point->x;
    cur_point.y = point->y;
    while (utf8[cur])
    {
        lib_font_render_char(canvas, &cur_point, font, utf8 + cur);
        cur_point.x += lib_font_get_char_width(font, utf8 + cur);
        cur += lib_utf8_char_len(utf8 + cur);
    }
}

void lib_font_render(CANVAS* canvas, RECT* rect, FONT* font, const char* utf8, unsigned int align)
{
    unsigned short width, height;
    POINT point;
    width = lib_font_get_text_width(font, utf8);
    height = font->height;
    if (width > rect->width)
        width = rect->width;
    if (height > rect->height)
        height = rect->height;
    switch (align & FONT_ALIGN_HMASK)
    {
    case FONT_ALIGN_RIGHT:
        point.x = rect->left + rect->width - width;
        break;
    case FONT_ALIGN_HCENTER:
        point.x = ((rect->width - width) >> 1) + rect->left;
        break;
    default:
        point.x = rect->left;
    }
    switch (align & FONT_ALIGN_VMASK)
    {
    case FONT_ALIGN_BOTTOM:
        point.y = rect->top + rect->height - height;
        break;
    case FONT_ALIGN_VCENTER:
        point.y = ((rect->height - height) >> 1) + rect->top;
        break;
    default:
        point.y = rect->top;
    }
    lib_font_render_text(canvas, &point, font, utf8);
}
