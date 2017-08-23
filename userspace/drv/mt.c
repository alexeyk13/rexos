/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "mt.h"
#include "../gpio.h"
#include "../stdio.h"
#include "../systime.h"

#define CLKS_TEST_ROUNDS                10000000

#define ADDSET_MASK                     (MT_CS1 | MT_CS2 | MT_RW | MT_A)

#define MT_PAGES_COUNT                  8
#define MT_SIZE_X                       64
#define MT_SIZE_Y                       128

#define MT_STATUS_RESET                 (1 << 4)
#define MT_STATUS_OFF                   (1 << 5)
#define MT_STATUS_BUSY                  (1 << 7)

#define MT_CMD_DISPLAY_ON               0x3f
#define MT_CMD_DISPLAY_OFF              0x3e
#define MT_CMD_START_LINE               0xc0
#define MT_CMD_DISPLAY_OFF              0x3e
#define MT_CMD_SET_PAGE                 0xb8
#define MT_CMD_SET_ADDRESS              0x40

#if (X_MIRROR)

#define X_TRANSFORM(x, size)            (MT_SIZE_X - (x) - (size))
#define PAGE_TRANSFORM(page)            (MT_PAGES_COUNT - (page) - 1)
#define BIT_OUT_TRANSFORM(byte)         (byte)

#else

#define X_TRANSFORM(x, size)            (x)
#define PAGE_TRANSFORM(page)            (page)
#define BIT_OUT_TRANSFORM(byte)         (mt_bitswap(byte))

#endif

uint8_t mt_bitswap(uint8_t x)
{
    x = ((x & 0x55) << 1) | ((x & 0xAA) >> 1);
    x = ((x & 0x33) << 2) | ((x & 0xCC) >> 2);
    x = ((x & 0x0F) << 4) | ((x & 0xF0) >> 4);
    return x;
}

static void delay_clks(unsigned int clks)
{
    int i;
    for (i = 0; i < clks; ++i)
        __NOP();
}

static uint8_t mt_read(unsigned int mask)
{
#if (TW)
    delay_clks(TW);
#endif //TW

    gpio_reset_mask(ADDSET_PORT, ADDSET_MASK);
    gpio_set_mask(ADDSET_PORT, mask | MT_RW);
    gpio_set_data_in(DATA_PORT, 8);

#if (TAS)
    delay_clks(TAS);
#endif //TAS
    gpio_set_pin(MT_STROBE);

#if (TDDR)
    delay_clks(TDDR);
#endif //TDDR
    uint8_t res = gpio_get_mask(DATA_PORT, DATA_MASK) & 0xff;

#if (PW)
    delay_clks(PW - TDDR);
#endif //PW
    gpio_reset_pin(MT_STROBE);
    return res;
}

static uint8_t mt_status(unsigned int cs)
{
    return mt_read(cs);
}

static void mt_write(unsigned int mask, uint8_t data)
{
#if (TW)
    delay_clks(TW);
#endif //TW
    gpio_reset_mask(ADDSET_PORT, ADDSET_MASK);
    gpio_set_mask(ADDSET_PORT, mask);
    gpio_set_data_out(DATA_PORT, 8);
    gpio_reset_mask(DATA_PORT, DATA_MASK);
    gpio_set_mask(DATA_PORT, data);

#if (TAS)
    delay_clks(TAS);
#endif //TAS
    gpio_set_pin(MT_STROBE);

#if (PW)
    delay_clks(PW);
#endif //PW
    gpio_reset_pin(MT_STROBE);
}

static void mt_cmd(unsigned int cs, uint8_t cmd)
{
    mt_write(cs, cmd);
    while(mt_status(cs) & MT_STATUS_BUSY) {}
}

static void mt_dataout(unsigned int cs, uint8_t data)
{
    mt_write(cs | MT_A, data);
    while(mt_status(cs) & MT_STATUS_BUSY) {}
}

static uint8_t mt_datain(unsigned int cs)
{
    return mt_read(cs | MT_A);
}

#if (MT_BACKLIGHT_CONTROL)
void mt_set_backlight(bool on)
{
    on ? gpio_set_pin(MT_BACKLIGHT) : gpio_reset_pin(MT_BACKLIGHT);
}
#endif //MT_BACKLIGHT_CONTROL

void mt_cls()
{
    unsigned int page, addr;
    for (page = 0; page < MT_PAGES_COUNT; ++page)
    {
        mt_cmd(MT_CS1, MT_CMD_SET_PAGE | page);
        mt_cmd(MT_CS1, MT_CMD_SET_ADDRESS | 0);
        for (addr = 0; addr < MT_SIZE_X; ++ addr)
            mt_dataout(MT_CS1, 0x00);
        mt_cmd(MT_CS2, MT_CMD_SET_PAGE | page);
        mt_cmd(MT_CS2, MT_CMD_SET_ADDRESS | 0);
        for (addr = 0; addr < MT_SIZE_X; ++ addr)
            mt_dataout(MT_CS2, 0x00);
    }
}

void mt_reset()
{
    gpio_reset_pin(MT_RESET);
    delay_clks(TRI);
    gpio_set_pin(MT_RESET);
    delay_clks(TR);
}

void mt_show(bool on)
{
    if (on)
    {
        mt_reset();
        mt_cls();
        mt_cmd(MT_CS1 | MT_CS2, MT_CMD_DISPLAY_ON);
    }
    else
    {
        mt_cmd(MT_CS1 | MT_CS2, MT_CMD_DISPLAY_OFF);
        //sending off is not enough, due to bug, display is still sucking energy from data pins
        gpio_set_data_out(DATA_PORT, 8);
        gpio_reset_mask(DATA_PORT, DATA_MASK);
        gpio_reset_mask(ADDSET_PORT, ADDSET_MASK);
        gpio_reset_pin(MT_RESET);
        gpio_reset_pin(MT_STROBE);
    }
}

bool mt_is_on()
{
    return !(mt_status(MT_CS1) & MT_STATUS_OFF);
}

#if (MT_TEST)
void mt_clks_test()
{
    TIME uptime;
    get_uptime(&uptime);
    delay_clks(CLKS_TEST_ROUNDS);
    printf("clks average time: %dns\n", systime_elapsed_us(&uptime) * 1000 / CLKS_TEST_ROUNDS);
}

static inline void mt_set_pixel(unsigned int x, unsigned int y, bool set)
{
    uint8_t data;
    unsigned int cs, page, xr, yr;
    if (x >= MT_SIZE_X || y >= MT_SIZE_Y)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    //find page & CS
    xr = X_TRANSFORM(x, 1);
    page = xr >> 3;
    if (y >= MT_SIZE_X)
    {
        yr = y - MT_SIZE_X;
        cs = MT_CS2;
    }
    else
    {
        yr = y;
        cs = MT_CS1;
    }
    mt_cmd(cs, MT_CMD_SET_PAGE | page);
    mt_cmd(cs, MT_CMD_SET_ADDRESS | yr);
    mt_datain(cs);
    data = mt_datain(cs);
    if (set)
        data |= 1 << (xr & 7);
    else
        data &= ~(1 << (xr & 7));
    mt_cmd(cs, MT_CMD_SET_ADDRESS | yr);
    mt_dataout(cs, data);
}

static void mt_poly_test(unsigned int y, unsigned int size)
{
    int i, j;
    for (i = 0; i < MT_SIZE_X; ++i)
    {
        for (j = y; j < y + size; ++j)
            mt_set_pixel(i, j, true);
    }
}

void mt_pixel_test()
{
    unsigned int off, sz;
    off = 0;
    sz = 8;
    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 6;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 5;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 4;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 3;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 2;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
    sz = 1;

    mt_poly_test(off, sz);
    off += sz * 2;

    mt_poly_test(off, sz);
    off += sz * 2;
}
#endif //MT_TEST

//here goes in each chip coords
static void mt_clear_rect_cs(unsigned int cs, RECT* rect)
{
    unsigned int first_page, last_page, page, i;
    first_page = rect->left >> 3;
    last_page = ((rect->left + rect->width + 7) >> 3) - 1;

    for (page = first_page; page <= last_page; ++page)
    {
        mt_cmd(cs, MT_CMD_SET_PAGE | PAGE_TRANSFORM(page));
        mt_cmd(cs, MT_CMD_SET_ADDRESS | rect->top);
        for (i = 0; i < rect->height; ++i)
            mt_dataout(cs, 0x00);
    }
}

void mt_clear_rect(const RECT *rect)
{
    RECT csrect;
    if (rect->left >= MT_SIZE_X || rect->top >= MT_SIZE_Y)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    csrect.width = rect->width;
    if (rect->left + rect->width > MT_SIZE_X)
        csrect.width = MT_SIZE_X - rect->left;
    csrect.left = rect->left;
    if (rect->left + rect->width > MT_SIZE_X)
        csrect.width = MT_SIZE_X - rect->left;
    //apply for CS1
    if (rect->top < MT_SIZE_X)
    {
        csrect.top = rect->top;
        csrect.height = rect->height;
        if (csrect.top + csrect.height > MT_SIZE_X)
            csrect.height = MT_SIZE_X - csrect.top;
        mt_clear_rect_cs(MT_CS1, &csrect);
    }
    //apply for CS2
    if (rect->top + rect->height > MT_SIZE_X)
    {
        if (rect->top < MT_SIZE_X)
        {
            csrect.top = 0;
            csrect.height = rect->top + rect->height - MT_SIZE_X;
        }
        else
        {
            csrect.top = rect->top - MT_SIZE_X;
            csrect.height = rect->height;
        }
        if (csrect.top + csrect.height > MT_SIZE_X)
            csrect.height = MT_SIZE_X - csrect.top;
        mt_clear_rect_cs(MT_CS2, &csrect);
    }
}

//here goes in each chip coords
static void mt_read_rect_cs(unsigned int cs, RECT* rect, uint8_t* data, unsigned int bpl, unsigned int offset)
{
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
        mt_cmd(cs, MT_CMD_SET_PAGE | PAGE_TRANSFORM(page));
        mt_cmd(cs, MT_CMD_SET_ADDRESS | rect->top);
        mt_datain(cs);
        for (i = 0; i < rect->height; ++i)
        {
            byte = mt_datain(cs);
            byte = BIT_OUT_TRANSFORM(byte) & mask;
            //absolute first bit offset in data stream
            cur = offset + i * bpl + ((page - first_page) << 3) + 8 - shift;
            byte_pos = cur >> 3;
            bit_pos = cur & 7;
            data[byte_pos] = (data[byte_pos] & ~((1 << bit_pos) - 1)) | (byte >> bit_pos);
            if (bit_pos)
                data[byte_pos + 1] = ((data[byte_pos + 1]  >> (8 - bit_pos)) << (8 - bit_pos)) | (byte & ~((1 << bit_pos) - 1));
        }
    }
}

static inline void mt_read_rect(RECT* rect, uint8_t *data)
{
    RECT csrect;
    //bits per line
    unsigned int offset;
    if (rect->left >= MT_SIZE_X || rect->top >= MT_SIZE_Y)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    offset = 0;
    csrect.width = rect->width;
    if (rect->left + rect->height > MT_SIZE_X)
        csrect.width = MT_SIZE_X - rect->left;
    csrect.left = rect->left;
    if (rect->left + rect->width > MT_SIZE_X)
        csrect.width = MT_SIZE_X - rect->left;
    //apply for CS1
    if (rect->top < MT_SIZE_X)
    {
        csrect.top = rect->top;
        csrect.height = rect->height;
        if (csrect.top + rect->height > MT_SIZE_X)
            csrect.height = MT_SIZE_X - rect->top;
        offset = rect->width * csrect.height;
        mt_read_rect_cs(MT_CS1, &csrect, data, rect->width, 0);
    }
    //apply for CS2
    if (rect->top + rect->height > MT_SIZE_X)
    {
        if (rect->top < MT_SIZE_X)
        {
            csrect.top = 0;
            csrect.height = rect->height - (MT_SIZE_X - rect->top);
        }
        else
        {
            csrect.top = rect->top - MT_SIZE_X;
            csrect.height = rect->height;
        }
        if (csrect.top + rect->height > MT_SIZE_X)
            csrect.height = MT_SIZE_X - rect->top;
        mt_read_rect_cs(MT_CS2, &csrect, data, rect->width, offset);
    }
}

//here goes in each chip coords
static void mt_write_rect_cs(unsigned int cs, RECT* rect, const uint8_t* data, unsigned int bpl, unsigned int offset)
{
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
            mt_dataout(cs, byte);
        }
    }
}

void mt_write_rect(const RECT* rect, const uint8_t* data)
{
    RECT csrect;
    //bits per line
    unsigned int offset;
    if (rect->left >= MT_SIZE_X || rect->top >= MT_SIZE_Y)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    offset = 0;
    csrect.width = rect->width;
    if (rect->left + rect->height > MT_SIZE_X)
        csrect.width = MT_SIZE_X - rect->left;
    csrect.left = rect->left;
    if (rect->left + rect->width > MT_SIZE_X)
        csrect.width = MT_SIZE_X - rect->left;
    //apply for CS1
    if (rect->top < MT_SIZE_X)
    {
        csrect.top = rect->top;
        csrect.height = rect->height;
        if (csrect.top + rect->height > MT_SIZE_X)
            csrect.height = MT_SIZE_X - rect->top;
        offset = rect->width * csrect.height;
        mt_write_rect_cs(MT_CS1, &csrect, data, rect->width, 0);
    }
    //apply for CS2
    if (rect->top + rect->height > MT_SIZE_X)
    {
        if (rect->top < MT_SIZE_X)
        {
            csrect.top = 0;
            csrect.height = rect->height - (MT_SIZE_X - rect->top);
        }
        else
        {
            csrect.top = rect->top - MT_SIZE_X;
            csrect.height = rect->height;
        }
        if (csrect.top + rect->height > MT_SIZE_X)
            csrect.height = MT_SIZE_X - rect->top;
        mt_write_rect_cs(MT_CS2, &csrect, data, rect->width, offset);
    }
}

void mt_read_canvas(CANVAS* canvas, const POINT* point)
{
    RECT rect;
    rect.left = point->x;
    rect.top = point->y;
    rect.width = canvas->width;
    rect.height = canvas->height;
    mt_read_rect(&rect, canvas->data);
}

void mt_write_canvas(CANVAS* canvas, const POINT* point)
{
    RECT rect;
    rect.left = point->x;
    rect.top = point->y;
    rect.width = canvas->width;
    rect.height = canvas->height;
    mt_write_rect(&rect, canvas->data);
}

void mt_enable()
{
    gpio_enable_mask(DATA_PORT, GPIO_MODE_OUT, DATA_MASK);
    gpio_enable_mask(ADDSET_PORT, GPIO_MODE_OUT, ADDSET_MASK);
    gpio_enable_pin(MT_RESET, GPIO_MODE_OUT);
    gpio_enable_pin(MT_STROBE, GPIO_MODE_OUT);
    gpio_reset_pin(MT_STROBE);
#if (MT_BACKLIGHT_CONTROL)
    gpio_enable_pin(MT_BACKLIGHT, GPIO_MODE_OUT);
#endif //MT_BACKLIGHT_CONTROL
}

void mt_disable()
{
    gpio_disable_mask(DATA_PORT, DATA_MASK);
    gpio_disable_mask(ADDSET_PORT, ADDSET_MASK);
    gpio_disable_pin(MT_RESET);
    gpio_disable_pin(MT_STROBE);
#if (MT_BACKLIGHT_CONTROL)
    gpio_disable_pin(MT_BACKLIGHT, GPIO_MODE_OUT);
#endif //MT_BACKLIGHT_CONTROL
}
