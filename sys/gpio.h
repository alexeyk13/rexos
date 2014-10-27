/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_H
#define GPIO_H

#include "sys.h"

typedef enum {
    GPIO_ENABLE_PIN = HAL_IPC(HAL_GPIO),
    GPIO_DISABLE_PIN,
    GPIO_SET_PIN,
    GPIO_GET_PIN,

    GPIO_HAL_MAX
} GPIO_IPCS;

typedef enum {
    PIN_MODE_OUT = 0,
    PIN_MODE_IN_FLOAT,
    PIN_MODE_IN_PULLUP,
    PIN_MODE_IN_PULLDOWN
} PIN_MODE;


#endif // GPIO_H
