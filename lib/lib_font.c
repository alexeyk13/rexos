/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_font.h"
#include "lib_graphics.h"
#include "../userspace/error.h"
#include "../userspace/heap.h"

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
