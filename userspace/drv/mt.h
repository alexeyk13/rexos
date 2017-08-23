/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT_H
#define MT_H

#include <stdbool.h>
#include <stdint.h>
#include "../process.h"
#include "../sys.h"
#include "../canvas.h"
#include "../graphics.h"
#include "mt_config.h"

#define MT_MODE_IGNORE                     0x0
#define MT_MODE_OR                         0x1
#define MT_MODE_XOR                        0x2
#define MT_MODE_AND                        0x3
#define MT_MODE_FILL                       0x4

#if (MT_BACKLIGHT_CONTROL)
void mt_set_backlight(bool on);
#endif //MT_BACKLIGHT_CONTROL
void mt_show(bool on);
bool mt_is_on();
void mt_cls();
void mt_reset();
void mt_enable();
void mt_disable();
#if (MT_TEST)
void mt_clks_test();
void mt_pixel_test();
#endif
void mt_clear_rect(const RECT* rect);
void mt_write_rect(const RECT *rect, const uint8_t *data);
void mt_read_canvas(CANVAS* canvas, const POINT* point);
void mt_write_canvas(CANVAS* canvas, const POINT *point);

#endif // MT_H
