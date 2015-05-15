/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_gpio.h"
#include "../../userspace/lpc_driver.h"
#include "lpc_config.h"
#include <stdint.h>

#define PIN_MODE_INVALID                0xf

#define IOCON                           ((uint32_t*)(LPC_IOCON_BASE))

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
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        GF(PIN_MODE_PIO1_0,  PIN_MODE_PIO1_1),  GF(PIN_MODE_PIO1_2,  PIN_MODE_PIO1_3),
                                        GF(PIN_MODE_PIO1_4,  PIN_MODE_PIO1_5),  GF(PIN_MODE_PIO1_6,  PIN_MODE_PIO1_7),
                                        GF(PIN_MODE_PIO1_8,  PIN_MODE_PIO1_9),  GF(PIN_MODE_PIO1_10, PIN_MODE_PIO1_11),
                                        GF(PIN_MODE_PIO1_12, PIN_MODE_PIO1_13), GF(PIN_MODE_PIO1_14, PIN_MODE_PIO1_15),
                                        GF(PIN_MODE_PIO1_16, PIN_MODE_PIO1_17), GF(PIN_MODE_PIO1_18, PIN_MODE_PIO1_19),
                                        GF(PIN_MODE_PIO1_20, PIN_MODE_PIO1_21), GF(PIN_MODE_PIO1_22, PIN_MODE_PIO1_23),
                                        GF(PIN_MODE_PIO1_24, PIN_MODE_PIO1_25), GF(PIN_MODE_PIO1_26, PIN_MODE_PIO1_27),
                                        GF(PIN_MODE_PIO1_28, PIN_MODE_PIO1_29), GF(PIN_MODE_INVALID, PIN_MODE_PIO1_31),
                                       };
#if (GPIO_TIMER)
static const uint8_t __GPIO_TIMER[PIN_DEFAULT / 2] =
                                       {
                                        //PIO0_0
                                        GF(PIN_MODE_INVALID,         PIN_MODE_CT32B0_MAT2),     GF(PIN_MODE_CT16B0_CAP0,    PIN_MODE_INVALID),
                                        //PIO0_4
                                        GF(PIN_MODE_INVALID,         PIN_MODE_INVALID),         GF(PIN_MODE_INVALID,        PIN_MODE_INVALID),
                                        //PIO0_8
                                        GF(PIN_MODE_CT16B0_MAT0,     PIN_MODE_CT16B0_MAT1),     GF(PIN_MODE_CT16B0_MAT2,    PIN_MODE_CT32B0_MAT3),
                                        //PIO0_12
                                        GF(PIN_MODE_CT32B1_CAP0,     PIN_MODE_CT32B1_MAT0),     GF(PIN_MODE_CT32B1_MAT1,    PIN_MODE_CT32B1_MAT2),
                                        //PIO0_16
                                        GF(PIN_MODE_CT32B1_MAT3,     PIN_MODE_CT32B0_CAP0),     GF(PIN_MODE_CT32B0_MAT0,    PIN_MODE_CT32B0_MAT1),
                                        //PIO0_20
                                        GF(PIN_MODE_CT16B1_CAP0,     PIN_MODE_CT16B1_MAT0),     GF(PIN_MODE_CT16B1_MAT1,    PIN_MODE_INVALID),
                                        //PIO0_24
                                        GF(PIN_MODE_INVALID,         PIN_MODE_INVALID),         GF(PIN_MODE_INVALID,        PIN_MODE_INVALID),
                                        //PIO0_28
                                        GF(PIN_MODE_INVALID,         PIN_MODE_INVALID),         GF(PIN_MODE_INVALID,        PIN_MODE_INVALID),
                                        //PIO1_0
                                        GF(PIN_MODE_CT32B1_MAT1_0,   PIN_MODE_CT32B1_MAT1_1),   GF(PIN_MODE_CT32B1_MAT2_1,  PIN_MODE_CT32B1_MAT3_1),
                                        //PIO1_4
                                        GF(PIN_MODE_CT32B1_CAP0_1_4, PIN_MODE_CT32B1_CAP1_1_5), GF(PIN_MODE_INVALID,        PIN_MODE_INVALID),
                                        //PIO1_8
                                        GF(PIN_MODE_INVALID,         PIN_MODE_INVALID),         GF(PIN_MODE_INVALID,        PIN_MODE_INVALID),
                                        //PIO1_12
                                        GF(PIN_MODE_INVALID,         PIN_MODE_CT16B0_MAT0_1),   GF(PIN_MODE_CT16B0_MAT1_1,  PIN_MODE_CT16B0_MAT2_1),
                                        //PIO1_16
                                        GF(PIN_MODE_CT16B0_CAP0_1,   PIN_MODE_CT16B0_CAP1_1),   GF(PIN_MODE_CT16B1_CAP1_1,  PIN_MODE_INVALID),
                                        //PIO1_20
                                        GF(PIN_MODE_INVALID,         PIN_MODE_INVALID),         GF(PIN_MODE_INVALID,        PIN_MODE_CT16B1_MAT1_1),
                                        //PIO1_24
                                        GF(PIN_MODE_CT32B0_MAT0_1,   PIN_MODE_CT32B0_MAT1_1),   GF(PIN_MODE_CT32B0_MAT2_1,  PIN_MODE_CT32B0_MAT3_1),
                                        //PIO1_28
                                        GF(PIN_MODE_CT32B0_CAP0_1,   PIN_MODE_CT32B0_CAP1_1),   GF(PIN_MODE_INVALID,        PIN_MODE_INVALID),
                                       };
#endif //GPIO_TIMER

void lpc_gpio_enable_pin(PIN pin, unsigned int mode, unsigned int af)
{
    if (pin >= PIN_DEFAULT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    unsigned int func = 0;
    switch (af)
    {
    case AF_IGNORE:
        break;
    case AF_DEFAULT:
#if (GPIO_HYSTERESIS)
        func = GPIO_FUNC(__GPIO_DEFAULT, pin) | GPIO_HYS;
#else
        func = GPIO_FUNC(__GPIO_DEFAULT, pin);
#endif
        break;
#if (GPIO_TIMER)
    case AF_TIMER:
        func = GPIO_FUNC(__GPIO_TIMER, pin);
        break;
#endif
    default:
        error(ERROR_INVALID_PARAMS);
        return;
    }
    //error invalid params
    if (func == PIN_MODE_INVALID)
        return;

    IOCON[pin] = (mode & LPC_GPIO_MODE_MASK) | func;
    if (mode & LPC_GPIO_MODE_OUT)
        LPC_GPIO->DIR[GPIO_PORT(pin)] |= 1 << GPIO_PIN(pin);
    else
        LPC_GPIO->DIR[GPIO_PORT(pin)] &= ~(1 << GPIO_PIN(pin));
}

__STATIC_INLINE void lpc_gpio_disable_pin(PIN pin)
{
    if (pin >= PIN_DEFAULT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    //default state, input
    IOCON[pin] = 0;
    LPC_GPIO->DIR[GPIO_PORT(pin)] &= ~(1 << GPIO_PIN(pin));
}

void lpc_gpio_init()
{
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << SYSCON_SYSAHBCLKCTRL_GPIO_POS) | (1 << SYSCON_SYSAHBCLKCTRL_IOCON_POS);
}

bool lpc_gpio_request(IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case LPC_GPIO_ENABLE_PIN:
        lpc_gpio_enable_pin((PIN)ipc->param1, ipc->param2, ipc->param3);
        need_post = true;
        break;
    case LPC_GPIO_DISABLE_PIN:
        lpc_gpio_disable_pin((PIN)ipc->param1);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
