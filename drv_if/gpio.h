/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_H
#define GPIO_H

#include "types.h"
#include "dev.h"

#ifdef CUSTOM_GPIO
#include "gpio_custom.h"
#else
#include "gpio_default.h"
#endif //GPIO_CUSTOM

typedef enum {
    //output, max speed
    PIN_MODE_OUT = 0,
    //input float. external resistor must be provided
    PIN_MODE_IN,
    //out open drain
    PIN_MODE_OUT_OD,
    //input with integrated pull-up/pull-down
    PIN_MODE_IN_PULLUP,
    PIN_MODE_IN_PULLDOWN
} PIN_MODE;

typedef enum {
    EXTI_MODE_RISING = 1,
    EXTI_MODE_FALLING  = 2,
    EXTI_MODE_BOTH = 3
} EXTI_MODE;

typedef void (*EXTI_HANDLER)(EXTI_CLASS);

extern void gpio_enable_pin(GPIO_CLASS pin, PIN_MODE mode);
extern void gpio_disable_pin(GPIO_CLASS pin);
extern void gpio_set_pin(GPIO_CLASS pin, bool set);
extern bool gpio_get_pin(GPIO_CLASS pin);
extern bool gpio_get_out_pin(GPIO_CLASS pin);

extern void gpio_exti_enable(EXTI_CLASS exti, EXTI_MODE mode, EXTI_HANDLER callback, int priority);
extern void gpio_exti_disable(EXTI_CLASS exti);

#endif //GPIO_H
