/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_GPIO_H
#define LPC_GPIO_H

#include "lpc_exo.h"

void lpc_gpio_enable_pin(GPIO gpio, GPIO_MODE mode);
void lpc_gpio_disable_pin(GPIO gpio);
void lpc_gpio_set_pin(GPIO gpio);
void lpc_gpio_reset_pin(GPIO gpio);

void lpc_gpio_request(IPC* ipc);

#endif // LPC_GPIO_H
