/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "mt.h"
#include "stm32/stm32_bitbang.h"
#include "../gpio.h"
#include "mt_config.h"

#define ADDSET_MASK                     (MT_CS1 | MT_CS2 | MT_RW | MT_A)


//Address hold time
#define TAS                             1
//Data read prepare time
#define TDDR                            17
//Delay between commands
#define TW                              256
//Reset time (max)
#define TR                              320
//Reset impulse time (max)
#define TRI                             32

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

__STATIC_INLINE void delay_clks(unsigned int clks)
{
    int i;
    for (i = 0; i < clks; ++i)
        __NOP();
}

__STATIC_INLINE uint8_t mt_read(unsigned int mask)
{
    delay_clks(TW);
    stm32_bitbang_set_data_in(DATA_PORT);
    stm32_bitbang_reset_mask(DATA_PORT, DATA_MASK);
    stm32_bitbang_reset_mask(ADDSET_PORT, ADDSET_MASK);
    stm32_bitbang_reset_pin(MT_STROBE);
    //Tah is only 20ns
    stm32_bitbang_set_mask(ADDSET_PORT, mask | MT_RW);
    delay_clks(TAS);
    stm32_bitbang_set_pin(MT_STROBE);
    delay_clks(TDDR);
    return stm32_bitbang_get_mask(DATA_PORT, DATA_MASK) & 0xff;
}

__STATIC_INLINE uint8_t mt_status(unsigned int cs)
{
    return mt_read(cs);
}

__STATIC_INLINE void mt_write(unsigned int mask, uint8_t data)
{
    delay_clks(TW);
    stm32_bitbang_set_data_out(DATA_PORT);
    stm32_bitbang_reset_mask(DATA_PORT, DATA_MASK);
    stm32_bitbang_reset_mask(ADDSET_PORT, ADDSET_MASK);
    stm32_bitbang_reset_pin(MT_STROBE);
    //Tah is only 20ns
    stm32_bitbang_set_mask(ADDSET_PORT, mask);
    delay_clks(TAS);
    stm32_bitbang_set_pin(MT_STROBE);
    stm32_bitbang_set_mask(DATA_PORT, data);
}

__STATIC_INLINE void mt_cmd(unsigned int cs, uint8_t cmd)
{
    mt_write(cs, cmd);
    while(mt_status(cs) & MT_STATUS_BUSY) {}
}

__STATIC_INLINE void mt_dataout(unsigned int cs, uint8_t data)
{
    mt_write(cs | MT_A, data);
    while(mt_status(cs) & MT_STATUS_BUSY) {}
}

__STATIC_INLINE uint8_t mt_datain(unsigned int cs)
{
    return mt_read(cs | MT_A);
}

void mt_set_backlight(bool on)
{
    gpio_set_pin(MT_BACKLIGHT, on);
}

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
    stm32_bitbang_reset_pin(MT_RESET);
    delay_clks(TRI);
    stm32_bitbang_set_pin(MT_RESET);
    delay_clks(TR);
}

void mt_show(bool on)
{
    if (on)
        mt_cmd(MT_CS1 | MT_CS2, MT_CMD_DISPLAY_ON);
    else
        mt_cmd(MT_CS1 | MT_CS2, MT_CMD_DISPLAY_OFF);
}

void mt_init()
{
    stm32_bitbang_enable_mask(DATA_PORT, DATA_MASK);
    stm32_bitbang_enable_mask(ADDSET_PORT, ADDSET_MASK);
    stm32_bitbang_enable_pin(MT_RESET);
    stm32_bitbang_enable_pin(MT_STROBE);
    stm32_bitbang_set_pin(MT_STROBE);
    //doesn't need to be so fast as others
    gpio_enable_pin(MT_BACKLIGHT, PIN_MODE_OUT);

    mt_reset();
    mt_cls();

}

void mt_set_pixel(unsigned int x, unsigned int y, bool set)
{
    uint8_t data;
    unsigned int cs, page, xr, yr;
    if (x >= MT_SIZE_X || y >= MT_SIZE_Y)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    //find page & CS
    xr = MT_SIZE_X - x - 1;
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

bool mt_get_pixel(unsigned int x, unsigned int y)
{
    uint8_t data;
    unsigned int cs, page, xr, yr;
    if (x >= MT_SIZE_X || y >= MT_SIZE_Y)
    {
        error(ERROR_OUT_OF_RANGE);
        return false;
    }
    //find page & CS
    xr = MT_SIZE_X - x - 1;
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
    return (data >> (xr & 7)) & 1;
}

void mt_pixel_test()
{
    unsigned int i, j, off, sz, k;
    off = 0;
    sz = 8;
    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;
    sz = 6;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;
    sz = 5;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;
    sz = 4;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;
    sz = 3;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;
    sz = 2;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;


    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;
    sz = 1;

    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;


    for (i = 0; i < MT_SIZE_X; i += 8)
    {
        for (j = off; j < off + sz; ++j)
            for (k = 0; k < 8; ++k)
                mt_set_pixel(i + k, j, true);
    }
    off += sz * 2;
}

void mt_image_test(const uint8_t* image)
{
    unsigned int page, y;

    for (page = 0; page < MT_PAGES_COUNT; ++page)
    {
        mt_cmd(MT_CS1, MT_CMD_SET_PAGE | page);
        mt_cmd(MT_CS1, MT_CMD_SET_ADDRESS | 0);
        for (y = 0; y < MT_SIZE_X; ++y)
            mt_dataout(MT_CS1, image[y * MT_PAGES_COUNT + MT_PAGES_COUNT - page - 1]);
    }
    for (page = 0; page < MT_PAGES_COUNT; ++page)
    {
        mt_cmd(MT_CS2, MT_CMD_SET_PAGE | page);
        mt_cmd(MT_CS2, MT_CMD_SET_ADDRESS | 0);
        for (y = 0; y < MT_SIZE_X; ++y)
            mt_dataout(MT_CS2, image[(y + MT_SIZE_X) * MT_PAGES_COUNT + MT_PAGES_COUNT - page - 1]);
    }
}
