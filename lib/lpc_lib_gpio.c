/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_gpio.h"
#include "../userspace/lpc_driver.h"
#include "lpc_config.h"

#define GPIO_FUNC(arr, pin)             (((arr)[(pin) >> 1] >> (((pin) & 1) << 2)) & 0xf)
#define GF(low, high)                   ((low) | ((high) << 4))

static const uint8_t __GPIO_DEFAULT[PIN_DEFAULT / 2] =
                                       {
                                        GF(PIN_MODE_PIO0_0,  PIN_MODE_PIO0_1),  GF(PIN_MODE_PIO0_2,  PIN_MODE_PIO0_3),
                                        GF(PIN_MODE_PIO0_4,  PIN_MODE_PIO0_5),  GF(PIN_MODE_PIO0_6,  PIN_MODE_PIO0_7),
                                        GF(PIN_MODE_PIO0_8,  PIN_MODE_PIO0_9),  GF(PIN_MODE_PIO0_10, PIN_MODE_PIO0_11),
                                        GF(PIN_MODE_PIO0_12, PIN_MODE_PIO0_13), GF(PIN_MODE_PIO0_14, PIN_MODE_PIO0_15),
                                        GF(PIN_MODE_PIO0_16, PIN_MODE_PIO0_17), GF(PIN_MODE_PIO0_18, PIN_MODE_PIO0_19),
                                        GF(PIN_MODE_PIO0_20, PIN_MODE_PIO0_21), GF(PIN_MODE_PIO0_22, PIN_MODE_PIO0_23),
                                        GF(0,                0),                GF(0,                0),
                                        GF(0,                0),                GF(0,                0),
                                        GF(PIN_MODE_PIO1_0,  PIN_MODE_PIO1_1),  GF(PIN_MODE_PIO1_2,  PIN_MODE_PIO1_3),
                                        GF(PIN_MODE_PIO1_4,  PIN_MODE_PIO1_5),  GF(PIN_MODE_PIO1_6,  PIN_MODE_PIO1_7),
                                        GF(PIN_MODE_PIO1_8,  PIN_MODE_PIO1_9),  GF(PIN_MODE_PIO1_10, PIN_MODE_PIO1_11),
                                        GF(PIN_MODE_PIO1_12, PIN_MODE_PIO1_13), GF(PIN_MODE_PIO1_14, PIN_MODE_PIO1_15),
                                        GF(PIN_MODE_PIO1_16, PIN_MODE_PIO1_17), GF(PIN_MODE_PIO1_18, PIN_MODE_PIO1_19),
                                        GF(PIN_MODE_PIO1_20, PIN_MODE_PIO1_21), GF(PIN_MODE_PIO1_22, PIN_MODE_PIO1_23),
                                        GF(PIN_MODE_PIO1_24, PIN_MODE_PIO1_25), GF(PIN_MODE_PIO1_26, PIN_MODE_PIO1_27),
                                        GF(PIN_MODE_PIO1_28, PIN_MODE_PIO1_29), GF(0,                PIN_MODE_PIO1_31),
                                       };


void lpc_lib_gpio_enable_pin(unsigned int pin, GPIO_MODE mode)
{
    unsigned int func;
#if (GPIO_HYSTERESIS)
    func = GPIO_FUNC(__GPIO_DEFAULT, pin) | GPIO_HYS;
#else
    func = GPIO_FUNC(__GPIO_DEFAULT, pin);
#endif
    switch (mode)
    {
    case GPIO_MODE_OUT:
        ack(object_get(SYS_OBJ_CORE), HAL_CMD(HAL_GPIO, LPC_GPIO_ENABLE_PIN), pin, LPC_GPIO_MODE_OUT | func, 0);
        break;
    case GPIO_MODE_IN_FLOAT:
        ack(object_get(SYS_OBJ_CORE), HAL_CMD(HAL_GPIO, LPC_GPIO_ENABLE_PIN), pin, GPIO_MODE_NOPULL | func, 0);
        break;
    case GPIO_MODE_IN_PULLUP:
        ack(object_get(SYS_OBJ_CORE), HAL_CMD(HAL_GPIO, LPC_GPIO_ENABLE_PIN), pin, GPIO_MODE_PULLUP | func, 0);
        break;
    case GPIO_MODE_IN_PULLDOWN:
        ack(object_get(SYS_OBJ_CORE), HAL_CMD(HAL_GPIO, LPC_GPIO_ENABLE_PIN), pin, GPIO_MODE_PULLDOWN | func, 0);
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
    ack(object_get(SYS_OBJ_CORE), HAL_CMD(HAL_GPIO, LPC_GPIO_DISABLE_PIN), pin, 0, 0);
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
