/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HIDD_H
#define HIDD_H

#include "usbd.h"

/*
    USB HID device driver.
    Mind ghe following limitations:

    1. Only one report is supported
    2. No Push/pull are supported
    3. Only single interface HID is supported
    4. Only CONTROL OUT pipe supported
 */

extern const USBD_CLASS __HIDD_CLASS;

#endif // HIDD_H
