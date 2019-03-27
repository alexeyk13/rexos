/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef HIDD_KBD_H
#define HIDD_KBD_H

#include "usbd.h"

/*
    USB HID keyboard device driver.
    Mind ghe following limitations:

    1. No custom REPORT descriptor is supported
    2. Only standart 101 key boot keyboard is supported
    3. No IDLE report is supported
 */

extern const USBD_CLASS __HIDD_KBD_CLASS;

#endif // HIDD_KBD_H
