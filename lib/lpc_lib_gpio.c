/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_gpio.h"
#include "../userspace/lpc_driver.h"

void lpc_lib_gpio_enable_pin(unsigned int pin, GPIO_MODE mode)
{
    switch (mode)
    {
    case GPIO_MODE_OUT:
        ack(object_get(SYS_OBJ_CORE), LPC_GPIO_ENABLE_PIN, pin, LPC_GPIO_MODE_OUT, AF_DEFAULT);
        break;
    case GPIO_MODE_IN_FLOAT:
        ack(object_get(SYS_OBJ_CORE), LPC_GPIO_ENABLE_PIN, pin, GPIO_MODE_NOPULL, AF_DEFAULT);
        break;
    case GPIO_MODE_IN_PULLUP:
        ack(object_get(SYS_OBJ_CORE), LPC_GPIO_ENABLE_PIN, pin, GPIO_MODE_PULLUP, AF_DEFAULT);
        break;
    case GPIO_MODE_IN_PULLDOWN:
        ack(object_get(SYS_OBJ_CORE), LPC_GPIO_ENABLE_PIN, pin, GPIO_MODE_PULLDOWN, AF_DEFAULT);
        break;
    }
}

void lpc_lib_gpio_enable_mask(unsigned int port, GPIO_MODE mode, unsigned int mask)
{
    unsigned int bit;
    unsigned int cur = mask;
    while (cur)
    {
        bit = 31 - __builtin_clz(cur);
        cur &= ~(1 << bit);
        lpc_lib_gpio_enable_pin(GPIO_MAKE_PIN(port, bit), mode);
    }
}

void lpc_lib_gpio_disable_pin(unsigned int pin)
{
    ack(object_get(SYS_OBJ_CORE), LPC_GPIO_DISABLE_PIN, pin, 0, 0);
}

void lpc_lib_gpio_disable_mask(unsigned int port, unsigned int mask)
{
    unsigned int bit;
    unsigned int cur = mask;
    while (cur)
    {
        bit = 31 - __builtin_clz(cur);
        cur &= ~(1 << bit);
        lpc_lib_gpio_disable_pin(GPIO_MAKE_PIN(port, bit));
    }
}

void lpc_lib_gpio_set_pin(unsigned int pin)
{
    LPC_GPIO->B[pin] = 1;
}

void lpc_lib_gpio_set_mask(unsigned int port, unsigned int mask)
{
    LPC_GPIO->SET[port] = mask;
}

void lpc_lib_gpio_reset_pin(unsigned int pin)
{
    LPC_GPIO->B[pin] = 0;
}

void lpc_lib_gpio_reset_mask(unsigned int port, unsigned int mask)
{
    LPC_GPIO->CLR[port] = mask;
}

bool lpc_lib_gpio_get_pin(unsigned int pin)
{
    return LPC_GPIO->B[pin] & 1;
}

unsigned int lpc_lib_gpio_get_mask(unsigned port, unsigned int mask)
{
    return LPC_GPIO->PIN[port] & mask;
}

void lpc_lib_gpio_set_data_out(unsigned int port, unsigned int wide)
{
    LPC_GPIO->DIR[port] |= (1 << wide) - 1;
}

void lpc_lib_gpio_set_data_in(unsigned int port, unsigned int wide)
{
    LPC_GPIO->DIR[port] &= ~((1 << wide) - 1);
}

const LIB_GPIO __LIB_GPIO = {
    lpc_lib_gpio_enable_pin,
    lpc_lib_gpio_enable_mask,
    lpc_lib_gpio_disable_pin,
    lpc_lib_gpio_disable_mask,
    lpc_lib_gpio_set_pin,
    lpc_lib_gpio_set_mask,
    lpc_lib_gpio_reset_pin,
    lpc_lib_gpio_reset_mask,
    lpc_lib_gpio_get_pin,
    lpc_lib_gpio_get_mask,
    lpc_lib_gpio_set_data_out,
    lpc_lib_gpio_set_data_in
};
