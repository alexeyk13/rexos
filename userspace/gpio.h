/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_H
#define GPIO_H

#include "sys.h"
#include <stdbool.h>
#include "lib.h"

typedef enum {
    GPIO_MODE_OUT = 0,
    GPIO_MODE_IN_FLOAT,
    GPIO_MODE_IN_PULLUP,
    GPIO_MODE_IN_PULLDOWN
} GPIO_MODE;

typedef struct {
    void (*lib_gpio_enable_pin)(unsigned int, GPIO_MODE);
    void (*lib_gpio_enable_mask)(unsigned int, GPIO_MODE, unsigned int);
    void (*lib_gpio_disable_pin)(unsigned int);
    void (*lib_gpio_disable_mask)(unsigned int, unsigned int);
    void (*lib_gpio_set_pin)(unsigned int);
    void (*lib_gpio_set_mask)(unsigned int, unsigned int);
    void (*lib_gpio_reset_pin)(unsigned int);
    void (*lib_gpio_reset_mask)(unsigned, unsigned int);
    bool (*lib_gpio_get_pin)(unsigned int);
    unsigned int (*lib_gpio_get_mask)(unsigned int, unsigned int);
    void (*lib_gpio_set_data_out)(unsigned int, unsigned int);
    void (*lib_gpio_set_data_in)(unsigned int, unsigned int);
} LIB_GPIO;


__STATIC_INLINE void gpio_enable_pin(unsigned int pin, GPIO_MODE mode)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_enable_pin(pin, mode);
}

__STATIC_INLINE void gpio_enable_mask(unsigned int port, GPIO_MODE mode, unsigned int mask)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_enable_mask(port, mode, mask);
}

__STATIC_INLINE void gpio_disable_pin(unsigned int pin)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_disable_pin(pin);
}

__STATIC_INLINE void gpio_disable_mask(unsigned int port, unsigned int mask)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_disable_mask(port, mask);
}

__STATIC_INLINE void gpio_set_pin(unsigned int pin)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_pin(pin);
}

__STATIC_INLINE void gpio_set_mask(unsigned int port, unsigned int mask)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_mask(port, mask);
}

__STATIC_INLINE void gpio_reset_pin(unsigned int pin)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_reset_pin(pin);
}

__STATIC_INLINE void gpio_reset_mask(unsigned int port, unsigned int mask)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_reset_mask(port, mask);
}

__STATIC_INLINE bool gpio_get_pin(unsigned int pin)
{
    LIB_CHECK_RET(LIB_ID_GPIO);
    return ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_get_pin(pin);
}

__STATIC_INLINE unsigned int gpio_get_mask(unsigned int port, unsigned int mask)
{
    LIB_CHECK_RET(LIB_ID_GPIO);
    return ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_get_mask(port, mask);
}

__STATIC_INLINE void gpio_set_data_out(unsigned int port, unsigned int wide)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_data_out(port, wide);
}

__STATIC_INLINE void gpio_set_data_in(unsigned int port, unsigned int wide)
{
    LIB_CHECK(LIB_ID_GPIO);
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_data_in(port, wide);
}

#endif // GPIO_H
