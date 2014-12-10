/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT_H
#define MT_H

#include <stdbool.h>
#include <stdint.h>

void mt_set_backlight(bool on);

void mt_set_pixel(unsigned int x, unsigned int y, bool set);
void mt_show(bool on);
void mt_init();
void mt_pixel_test();
void mt_image_test(const uint8_t* image);

#endif // MT_H
