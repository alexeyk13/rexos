/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "../graphics.h"
#include "../heap.h"
#include "../error.h"

static void graphics_write(uint8_t* pix, unsigned short pix_width, unsigned short bpp, POINT* point, unsigned int data, unsigned int data_width)
{
    unsigned short byte_pos, bit_pos, width_left, cur_width;
    unsigned int data_pos = (pix_width * point->y + point->x) * bpp;
    for (width_left = data_width, byte_pos = (data_pos >> 3) + 1; width_left; width_left -= cur_width, ++byte_pos, data_pos += cur_width)
    {
        bit_pos = data_pos & 7;
        cur_width = 8 - bit_pos;
        if (cur_width > width_left)
            cur_width = width_left;
        pix[byte_pos] &= ~(((1 << cur_width) - 1) << (8 - bit_pos - cur_width));
        pix[byte_pos] |= ((data >> (width_left - cur_width)) & ((1 << cur_width) - 1)) << (8 - bit_pos - cur_width);
    }
}

static unsigned int graphics_read(const uint8_t* pix, unsigned short pix_width, unsigned short bpp, POINT* point, unsigned int data_width)
{
    unsigned short byte_pos, bit_pos, width_left, cur_width;
    unsigned int data = 0;
    unsigned int data_pos = (pix_width * point->y + point->x) * bpp;
    for (width_left = data_width, byte_pos = (data_pos >> 3) + 1; width_left; width_left -= cur_width, ++byte_pos, data_pos += cur_width)
    {
        bit_pos = data_pos & 7;
        cur_width = 8 - bit_pos;
        if (cur_width > width_left)
            cur_width = width_left;
        data = data << cur_width;
        data |= (pix[byte_pos] >> (8 - bit_pos - cur_width)) & ((1 << cur_width) - 1);
    }
    return data;
}

void put_pixel(CANVAS* canvas, POINT* point, unsigned int color)
{
    if (point->x >= canvas->width || point->y >= canvas->height)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    graphics_write(CANVAS_DATA(canvas), canvas->width, canvas->bits_per_pixel, point, color, canvas->bits_per_pixel);
}

unsigned int get_pixel(CANVAS* canvas, POINT* point)
{
    if (point->x >= canvas->width || point->y >= canvas->height)
    {
        error(ERROR_OUT_OF_RANGE);
        return 0;
    }
    return graphics_read(CANVAS_DATA(canvas), canvas->width, canvas->bits_per_pixel, point, canvas->bits_per_pixel);
}

void clear_rect(CANVAS* canvas, RECT* rect)
{
    POINT point;
    unsigned short width, height, cur_width;
    unsigned int ppi = (sizeof(int) << 3) / canvas->bits_per_pixel;
    if (rect->left >= canvas->width || rect->top >= canvas->height)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    width = rect->width;
    if (rect->left + width > canvas->width)
        width = canvas->width - rect->left;
    height = rect->height;
    if (rect->top + height > canvas->height)
        height = canvas->height - rect->top;
    for (point.y = rect->top; point.y < rect->top + height; ++point.y)
    {
        for (point.x = rect->left; point.x < rect->left + width; point.x += cur_width)
        {
            cur_width = ppi;
            if (point.x + cur_width > rect->left + width)
                cur_width = rect->left + width - point.x;
            graphics_write(CANVAS_DATA(canvas), canvas->width, canvas->bits_per_pixel, &point, 0x0, cur_width * canvas->bits_per_pixel);
        }
    }
}

void write_rect(CANVAS* canvas, RECT* rect, RECT* data_rect, const uint8_t* pix, unsigned int mode)
{
    POINT point, data_point;
    unsigned short width, height, cur_width;
    unsigned int data;
    unsigned int ppi = (sizeof(int) << 3) / canvas->bits_per_pixel;
    if (rect->left >= canvas->width || rect->top >= canvas->height || data_rect->left >= data_rect->width || data_rect->top >= data_rect->height)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    width = rect->width;
    if (rect->left + width > canvas->width)
        width = canvas->width - rect->left;
    if (data_rect->left + width > data_rect->width)
        width = data_rect->width - data_rect->left;
    height = rect->height;
    if (rect->top + height > canvas->height)
        height = canvas->height - rect->top;
    if (data_rect->top + height > data_rect->height)
        height = data_rect->height - data_rect->top;
    for (point.y = rect->top; point.y < rect->top + height; ++point.y)
    {
        for (point.x = rect->left; point.x < rect->left + width; point.x += cur_width)
        {
            cur_width = ppi;
            if (point.x + cur_width > rect->left + width)
                cur_width = rect->left + width - point.x;
            data_point.x = point.x - rect->left + data_rect->left;
            data_point.y = point.y - rect->top + data_rect->top;
            data = graphics_read(pix, data_rect->width, canvas->bits_per_pixel, &data_point, cur_width * canvas->bits_per_pixel);
            switch (mode)
            {
            case GUI_MODE_OR:
                data |= graphics_read(CANVAS_DATA(canvas), canvas->width, canvas->bits_per_pixel, &point, cur_width * canvas->bits_per_pixel);
                break;
            case GUI_MODE_XOR:
                data ^= graphics_read(CANVAS_DATA(canvas), canvas->width, canvas->bits_per_pixel, &point, cur_width * canvas->bits_per_pixel);
                break;
            case GUI_MODE_AND:
                data &= graphics_read(CANVAS_DATA(canvas), canvas->width, canvas->bits_per_pixel, &point, cur_width * canvas->bits_per_pixel);
                break;
            default:
                break;
            }
            graphics_write(CANVAS_DATA(canvas), canvas->width, canvas->bits_per_pixel, &point, data, cur_width * canvas->bits_per_pixel);
        }
    }
}

