/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "mt.h"
#include "stm32/stm32_bitbang.h"
#include "../gpio.h"
#if (SYS_INFO)
#include "../../userspace/lib/stdio.h"
#include "../../userspace/timer.h"
#endif

#if (MT_DRIVER)
void mt();

const REX __MT = {
    //name
    "MT LCD driver",
    //size
    MT_STACK_SIZE,
    //priority - driver priority
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    MT_IPC_COUNT,
    //function
    mt
};
#endif

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

#if (MT_TEST)
static void mt_poly_test(unsigned int y, unsigned int size)
{
    for (i = 0; i < MT_SIZE_X; ++i)
    {
        for (j = y; j < y + size; ++j)
            mt_set_pixel(i, j, true);
    }
}

void mt_pixel_test()
{
    unsigned int i, j, off, sz;
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

#if (MT_DRIVER)
#if (SYS_INFO)
static inline void mt_info()
{
    TIME uptime;
    get_uptime(&uptime);

    mt_pixel_test();
    printf("pixel test time(us): %dus\n\r", time_elapsed_us(&uptime));
}
#endif

void mt()
{
    mt_init();
    IPC ipc;
#if (SYS_INFO)
    open_stdout();
#endif
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        case IPC_CALL_ERROR:
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
            mt_info(drv);
            need_post = true;
            break;
#endif
        case MT_RESET:
            mt_reset();
            need_post = true;
            break;
        case MT_SHOW:
            mt_show(ipc.param1);
            need_post = true;
            break;
        case MT_BACKLIGHT:
            mt_backlight(ipc.param1);
            need_post = true;
            break;
        case MT_CLS:
            mt_cls();
            need_post = true;
            break;
        case MT_SET_PIXEL:
            mt_set_pixel(ipc.param1, ipc.param2, ipc.param3);
            need_post = true;
            break;
        case MT_GET_PIXEL:
            ipc.param1 = mt_get_pixel(ipc.param1, ipc.param2);
            need_post = true;
            break;
#if (MT_TEST)
        case MT_PIXEL_TEST:
            mt_pixel_test();
            need_post = true;
            break;
#endif
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}

#endif
