/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "gpio.h"

void gpio_enable_pin(unsigned int pin, GPIO_MODE mode)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_enable_pin(pin, mode);
}

void gpio_enable_mask(unsigned int port, GPIO_MODE mode, unsigned int mask)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_enable_mask(port, mode, mask);
}

void gpio_disable_pin(unsigned int pin)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_disable_pin(pin);
}

void gpio_disable_mask(unsigned int port, unsigned int mask)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_disable_mask(port, mask);
}

void gpio_set_pin(unsigned int pin)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_pin(pin);
}

void gpio_set_mask(unsigned int port, unsigned int mask)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_mask(port, mask);
}

void gpio_reset_pin(unsigned int pin)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_reset_pin(pin);
}

void gpio_reset_mask(unsigned int port, unsigned int mask)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_reset_mask(port, mask);
}

bool gpio_get_pin(unsigned int pin)
{
    return ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_get_pin(pin);
}

unsigned int gpio_get_mask(unsigned int port, unsigned int mask)
{
    return ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_get_mask(port, mask);
}

void gpio_set_data_out(unsigned int port, unsigned int wide)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_data_out(port, wide);
}

void gpio_set_data_in(unsigned int port, unsigned int wide)
{
    ((const LIB_GPIO*)__GLOBAL->lib[LIB_ID_GPIO])->lib_gpio_set_data_in(port, wide);
}
