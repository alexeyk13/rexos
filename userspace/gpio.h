/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_H
#define GPIO_H

#include "sys.h"
#include "sys_config.h"
#include "cc_macro.h"
#include "ipc.h"

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

__STATIC_INLINE void gpio_enable_pin(int pin, PIN_MODE mode)
{
    ack(object_get(SYS_OBJ_CORE), GPIO_ENABLE_PIN, pin, mode, 0);
}

__STATIC_INLINE void gpio_disable_pin(int pin)
{
    ack(object_get(SYS_OBJ_CORE), GPIO_DISABLE_PIN, pin, 0, 0);
}

__STATIC_INLINE void gpio_set_pin(int pin, bool set)
{
    ack(object_get(SYS_OBJ_CORE), GPIO_SET_PIN, pin, set, 0);
}

__STATIC_INLINE bool gpio_get_pin(int pin)
{
    return get(object_get(SYS_OBJ_CORE), GPIO_GET_PIN, pin, 0, 0);
}

#endif // GPIO_H
