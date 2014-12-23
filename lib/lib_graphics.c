#include "lib_graphics.h"
#include "../userspace/heap.h"
#include "../userspace/error.h"
#include "../userspace/canvas.h"

static uint8_t bitswap(uint8_t x)
{
    x = ((x & 0x55) << 1) | ((x & 0xAA) >> 1);
    x = ((x & 0x33) << 2) | ((x & 0xCC) >> 2);
    x = ((x & 0x0F) << 4) | ((x & 0xF0) >> 4);
    return x;
}

void lib_graphics_put_pixel(CANVAS* canvas, unsigned short x, unsigned short y, unsigned int color)
{
    unsigned int pixelpos, byte, bit;
    if (x >= canvas->width || y >= canvas->height)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    pixelpos = (y * canvas->width + x) * canvas->bits_per_pixel;
    byte = (pixelpos >> 3) + 1;
    bit = 7 - (pixelpos & 7);
    if (color)
        CANVAS_DATA(canvas)[byte] |= 1 << bit;
    else
        CANVAS_DATA(canvas)[byte] &= ~(1 << bit);
}

unsigned int lib_graphics_get_pixel(CANVAS* canvas, unsigned short x, unsigned short y)
{
    unsigned int pixelpos, byte, bit;
    if (x >= canvas->width || y >= canvas->height)
    {
        error(ERROR_OUT_OF_RANGE);
        return 0;
    }
    pixelpos = (y * canvas->width + x) * canvas->bits_per_pixel;
    byte = (pixelpos >> 3) + 1;
    //faster than bitswap
    bit = 7 - (pixelpos & 7);
    return (CANVAS_DATA(canvas)[byte] >> bit) & 1;
}

static bool lib_graphics_normalize_rect(CANVAS* canvas, RECT* in, RECT* out)
{
    if (in->left >= canvas->width || in->top >= canvas->height)
    {
        error(ERROR_OUT_OF_RANGE);
        return false;
    }
    out->left = in->left;
    out->top = in->top;
    out->width = in->width;
    if (out->left + out->width > canvas->width)
        out->width = canvas->width - out->left;
    out->height = in->height;
    if (out->top + out->height > canvas->height)
        out->height = canvas->height - out->top;
    return true;
}

void lib_graphics_clear_rect(CANVAS* canvas, RECT* rect)
{
    RECT nrect;
    unsigned short line, column, cur_width, pixelpos, byte, bit;
    uint8_t mask;
    if (!lib_graphics_normalize_rect(canvas, rect, &nrect))
        return;
    for (line = nrect.top; line < nrect.top + nrect.height; ++line)
    {
        for (column = nrect.left; column < nrect.left + nrect.width; column += cur_width)
        {
            cur_width = 8 - (column & 7);
            if (column + cur_width > nrect.left + nrect.width)
                cur_width = nrect.left + nrect.width - column;
            pixelpos = (line * canvas->width + column) * canvas->bits_per_pixel;
            byte = (pixelpos >> 3) + 1;
            bit = (pixelpos & 7);
            mask = (0xff & ((1 << cur_width) - 1)) << bit;
            CANVAS_DATA(canvas)[byte] &= ~(bitswap(mask));
        }
    }
}

void lib_graphics_write_rect(CANVAS* canvas, RECT* rect, uint8_t* data, RECT* data_rect, unsigned int mode)
{

}


/*
static void mt_write_rect_cs(unsigned int cs, RECT* rect, const uint8_t* data, unsigned int bpl, unsigned int offset)
{
    uint8_t buf[rect->height];
    uint8_t byte, mask;
    uint8_t shift = 0;
    unsigned int first_page, last_page, page, i, cur, byte_pos, bit_pos;
    first_page = rect->left >> 3;
    last_page = ((rect->left + rect->width + 7) >> 3) - 1;
    shift = rect->left & 7;

    for (page = first_page; page <= last_page; ++page)
    {
        mask = 0xff;
        if (page == first_page)
            mask >>= shift;
        if (page == last_page)
            mask &= ~((1 << (((last_page + 1) << 3) - (rect->left + rect->width))) - 1);
        mask = BIT_OUT_TRANSFORM(mask);
        if (mode != MT_MODE_IGNORE)
        {
            mt_cmd(cs, MT_CMD_SET_PAGE | PAGE_TRANSFORM(page));
            mt_cmd(cs, MT_CMD_SET_ADDRESS | rect->top);
            mt_datain(cs);
            for (i = 0; i < rect->height; ++i)
                buf[i] = mt_datain(cs);
        }
        mt_cmd(cs, MT_CMD_SET_PAGE | PAGE_TRANSFORM(page));
        mt_cmd(cs, MT_CMD_SET_ADDRESS | rect->top);
        for (i = 0; i < rect->height; ++i)
        {
            //absolute first bit offset in data stream
            cur = offset + i * bpl + ((page - first_page) << 3) + 8 - shift;
            byte_pos = cur >> 3;
            bit_pos = cur & 7;
            byte = (data[byte_pos] << (bit_pos)) & 0xff;
            if (bit_pos)
                byte |= data[byte_pos + 1] >> (8 - bit_pos);
            byte = BIT_OUT_TRANSFORM(byte) & mask;
            switch (mode)
            {
            case MT_MODE_OR:
                mt_dataout(cs, buf[i] | byte);
                break;
            case MT_MODE_XOR:
                mt_dataout(cs, buf[i] ^ byte);
                break;
            case MT_MODE_FILL:
                mt_dataout(cs, (buf[i] & ~mask) | byte);
                break;
            default:
                mt_dataout(cs, byte);
            }
        }
    }
}

 */
