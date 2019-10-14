/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/


#include "../gpio.h"
#include "../pin.h"
#include "nrf_driver.h"

#if defined(NRF52)
#define NRF_GPIO                    NRF_P0
#endif // NRF52

void gpio_enable_pin(unsigned int pin, GPIO_MODE mode)
{
    switch (mode)
    {
        case GPIO_MODE_OUT:
            pin_enable(pin, PIN_MODE_OUTPUT, PIN_PULL_NOPULL);
            break;
        case GPIO_MODE_IN_FLOAT:
            pin_enable(pin, PIN_MODE_INPUT, PIN_PULL_NOPULL);
            break;
        case GPIO_MODE_IN_PULLUP:
            pin_enable(pin, PIN_MODE_INPUT, PIN_PULL_UP);
            break;
        case GPIO_MODE_IN_PULLDOWN:
            pin_enable(pin, PIN_MODE_INPUT, PIN_PULL_DOWN);
            break;
    }
}

void gpio_disable_pin(unsigned int pin)
{
    pin_disable(pin);
}

void gpio_set_pin(unsigned int pin)
{
    NRF_GPIO->OUTSET = (1 << pin);
}

void gpio_reset_pin(unsigned int pin)
{
    NRF_GPIO->OUTCLR = (1 << pin);
}

bool gpio_get_pin(unsigned int pin)
{
    return ((NRF_GPIO->IN >> pin) & 1);
}

unsigned int gpio_get_mask(unsigned port, unsigned int mask)
{
    // TODO:
    return 0;
}

void gpio_set_data_out(unsigned int port, unsigned int wide)
{
    // TODO:
}

void gpio_set_data_in(unsigned int port, unsigned int wide)
{
    // TODO:
}
