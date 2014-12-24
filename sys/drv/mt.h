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
#include "../../userspace/canvas.h"
#include "mt_config.h"

#define MT_MODE_IGNORE                     0x0
#define MT_MODE_OR                         0x1
#define MT_MODE_XOR                        0x2
#define MT_MODE_AND                        0x3
#define MT_MODE_FILL                       0x4

#if (MT_DRIVER)
typedef enum {
    MT_RESET = HAL_IPC(HAL_POWER),
    MT_SHOW,
    MT_BACKLIGHT
    MT_CLS,
    MT_CLEAR_RECT,
    MT_READ_CANVAS,
    MT_WRITE_RECT,
    MT_WRITE_CANVAS,
    MT_PIXEL_TEST
} MT_IPCS;

extern const REX __MT;

typedef struct {
    RECT rect;
    HANDLE block;
} MT_REQUEST;

#else

void mt_set_backlight(bool on);
void mt_show(bool on);
bool mt_is_on();
void mt_cls();
void mt_reset();
void mt_init();
#if (MT_TEST)
void mt_clks_test();
void mt_pixel_test();
#endif
void mt_clear_rect(RECT* rect);
void mt_write_rect(RECT* rect, const uint8_t *data);
void mt_read_canvas(CANVAS* canvas, POINT* point);
void mt_write_canvas(CANVAS* canvas, POINT* point);

#endif //MT_DRIVER

#endif // MT_H
