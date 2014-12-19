/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_GPIO_H
#define LPC_GPIO_H

#include <stdbool.h>
#include "../../../userspace/cc_macro.h"
#include "../../../userspace/sys.h"
#include "../../../userspace/gpio.h"

void lpc_gpio_init();
bool lpc_gpio_request(IPC* ipc);

__STATIC_INLINE unsigned int lpc_gpio_request_inside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    lpc_gpio_request(&ipc);
    return ipc.param1;
}

#endif // LPC_GPIO_H
