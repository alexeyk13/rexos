/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT_H
#define MT_H

#include <stdbool.h>
#include <stdint.h>
#include "../../userspace/process.h"
#include "../../userspace/sys.h"
#include "mt_config.h"

#define MT_MODE_IGNORE                     0x0
#define MT_MODE_OR                         0x1
#define MT_MODE_XOR                        0x2
#define MT_MODE_AND                        0x3
#define MT_MODE_FILL                       0x4

typedef struct {
    uint16_t left, top, width, height;
} RECT;

#if (MT_DRIVER)
typedef enum {
    MT_RESET = HAL_IPC(HAL_POWER),
    MT_SHOW,
    MT_BACKLIGHT
    MT_CLS,
    MT_SET_PIXEL,
    MT_GET_PIXEL,
    MT_CLEAR_RECT,
    MT_WRITE_RECT,
    MT_PIXEL_TEST
} MT_IPCS;

extern const REX __MT;

typedef struct {
    RECT rect;
    HANDLE block;
    uint16_t mode;
} MT_REQUEST;

#else

void mt_set_backlight(bool on);
void mt_set_pixel(unsigned int x, unsigned int y, bool set);
bool mt_get_pixel(unsigned int x, unsigned int y);
void mt_show(bool on);
bool mt_is_on();
void mt_cls();
void mt_reset();
void mt_init();
#if (MT_TEST)
void mt_clks_test();
void mt_pixel_test();
#endif
void mt_clear_rect(RECT* rect, unsigned int mode);
void mt_write_rect(RECT* rect, unsigned int mode, const uint8_t *data);

#endif //MT_DRIVER

#endif // MT_H
