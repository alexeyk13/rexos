/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_gpio.h"
#include "lpc_config.h"
#include <stdint.h>

#define PIN_MODE_INVALID                0xf

#define IOCON                           ((uint32_t*)(LPC_IOCON_BASE))

#define GPIO_FUNC(arr, pin)             (((arr)[(pin) >> 1] >> (((pin) & 1) << 2)) & 0xf)
#define GF(low, high)                   ((low) | ((high) << 4))

#define GPIO_PORT(pin)                  ((pin) >> 5)
#define GPIO_PIN(pin)                   ((pin) & 0x1f)

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
#if (GPIO_UART)
static const uint8_t __GPIO_UART[PIN_DEFAULT / 2] =
                                       {
                                        //PIO0_0
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO0_4
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_CTS),
                                        //PIO0_8
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO0_12
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO0_16
#if (GPIO_PIO0_17_SCLK)
                                        GF(PIN_MODE_INVALID, PIN_MODE_SCLK),    GF(PIN_MODE_RXD,     PIN_MODE_TXD),
#else
                                        GF(PIN_MODE_INVALID, RTS),              GF(PIN_MODE_RXD,     PIN_MODE_TXD),
#endif
                                        //PIO0_20
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO0_24
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO0_28
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO1_0
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO1_4
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO1_8
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                        //PIO1_12
#if (GPIO_PIO1_13_TXD) && (GPIO_PIO1_14_RXD)
                                        GF(PIN_MODE_INVALID, PIN_MODE_TXD_1),   GF(PIN_MODE_RXD_1,   PIN_MODE_DCD),
#elif (GPIO_PIO1_13_TXD)
                                        GF(PIN_MODE_INVALID, PIN_MODE_TXD_1),   GF(PIN_MODE_DSR,     PIN_MODE_DCD),
#elif (GPIO_PIO1_14_RXD)
                                        GF(PIN_MODE_INVALID, PIN_MODE_DTR),     GF(PIN_MODE_RXD_1,   PIN_MODE_DCD),
#else
                                        GF(PIN_MODE_INVALID, PIN_MODE_DTR),     GF(PIN_MODE_DSR,     PIN_MODE_DCD),
#endif
                                        //PIO1_16
                                        GF(PIN_MODE_RI,      PIN_MODE_RXD_2),   GF(PIN_MODE_TXD_2,   PIN_MODE_DTR_1),
                                        //PIO1_20
                                        GF(PIN_MODE_DSR_1,   PIN_MODE_DCD_1),   GF(PIN_MODE_RI_1,    PIN_MODE_INVALID),
                                        //PIO1_24
                                        GF(PIN_MODE_INVALID, PIN_MODE_INVALID), GF(PIN_MODE_RXD_3,   PIN_MODE_TXD_3),
                                        //PIO1_28
                                        GF(PIN_MODE_SCLK_1,  PIN_MODE_INVALID), GF(PIN_MODE_INVALID, PIN_MODE_INVALID),
                                       };
#endif

void lpc_gpio_enable_pin_system(PIN pin, unsigned int mode, unsigned int af)
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
#if (GPIO_UART)
    case AF_UART:
        func = GPIO_FUNC(__GPIO_UART, pin);
        break;
#endif
#if (GPIO_I2C)
    case AF_I2C:
        func = PIN_MODE_I2C_SCL | GPIO_I2C_MODE_STANDART;
        break;
    case AF_FAST_I2C:
        func = PIN_MODE_I2C_SCL | GPIO_I2C_MODE_FAST;
        break;
#endif
    default:
        error(ERROR_INVALID_PARAMS);
        return;
    }
    //error invalid params
    if (func == PIN_MODE_INVALID)
        return;

    IOCON[pin] = (mode & GPIO_MODE_MASK) | func;
    if (mode & GPIO_MODE_OUT)
        LPC_GPIO->DIR[GPIO_PORT(pin)] |= 1 << GPIO_PIN(pin);
    else
        LPC_GPIO->DIR[GPIO_PORT(pin)] &= ~(1 << GPIO_PIN(pin));
}

__STATIC_INLINE void lpc_gpio_enable_pin(PIN pin, PIN_MODE mode)
{
    switch (mode)
    {
    case PIN_MODE_OUT:
        lpc_gpio_enable_pin_system(pin, GPIO_MODE_OUT, AF_DEFAULT);
        break;
    case PIN_MODE_IN_FLOAT:
        lpc_gpio_enable_pin_system(pin, GPIO_MODE_NOPULL, AF_DEFAULT);
        break;
    case PIN_MODE_IN_PULLUP:
        lpc_gpio_enable_pin_system(pin, GPIO_MODE_PULLUP, AF_DEFAULT);
        break;
    case PIN_MODE_IN_PULLDOWN:
        lpc_gpio_enable_pin_system(pin, GPIO_MODE_PULLDOWN, AF_DEFAULT);
        break;
    }
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

__STATIC_INLINE void lpc_gpio_set_pin(PIN pin, bool set)
{
    if (pin >= PIN_DEFAULT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    LPC_GPIO->B[pin] = set ? 1 : 0;
}

__STATIC_INLINE bool lpc_gpio_get_pin(PIN pin)
{
    if (pin >= PIN_DEFAULT)
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }
    return LPC_GPIO->B[pin] & 1;
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
    case GPIO_ENABLE_PIN:
        lpc_gpio_enable_pin((PIN)ipc->param1, (PIN_MODE)ipc->param2);
        need_post = true;
        break;
    case GPIO_SET_PIN:
        lpc_gpio_set_pin((PIN)ipc->param1, (bool)ipc->param2);
        need_post = true;
        break;
    case GPIO_GET_PIN:
        ipc->param1 = lpc_gpio_get_pin((PIN)ipc->param1);
        ipc_post_or_error(ipc);
        break;
    case GPIO_DISABLE_PIN:
        lpc_gpio_disable_pin((PIN)ipc->param1);
        need_post = true;
        break;
    case LPC_GPIO_ENABLE_PIN_SYSTEM:
        lpc_gpio_enable_pin_system((PIN)ipc->param1, ipc->param2, ipc->param3);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
