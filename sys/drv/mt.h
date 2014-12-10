/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT_H
#define MT_H

#include <stdbool.h>
#include <stdint.h>
#include "../../../userspace/process.h"
#include "../sys.h"
#include "mt_config.h"

#if (MT_DRIVER)
typedef enum {
    MT_RESET = HAL_IPC(HAL_POWER),
    MT_SHOW,
    MT_BACKLIGHT
    MT_CLS,
    MT_SET_PIXEL,
    MT_GET_PIXEL,
    MT_PIXEL_TEST,
    MT_IMAGE_TEST
} MT_IPCS;

extern const REX __MT;

#else

void mt_set_backlight(bool on);
void mt_set_pixel(unsigned int x, unsigned int y, bool set);
bool mt_get_pixel(unsigned int x, unsigned int y);
void mt_show(bool on);
void mt_cls();
void mt_reset();
void mt_init();
#if (MT_TEST)
void mt_pixel_test();
#endif
void mt_image_test(const uint8_t* image);

#endif //MT_DRIVER

#endif // MT_H
