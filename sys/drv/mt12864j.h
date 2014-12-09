/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT12864J_H
#define MT12864J_H

#include <stdbool.h>

void mt12864j_set_backlight(bool on);

void mt12864j_set_pixel(unsigned int x, unsigned int y, bool set);
void mt12864j_init();

#endif // MT12864J_H
