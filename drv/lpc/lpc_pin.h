/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_PIN_H
#define LPC_PIN_H

#include <stdbool.h>
#include "../../userspace/cc_macro.h"
#include "../../userspace/gpio.h"
#include "../../userspace/ipc.h"

void lpc_pin_init();
void lpc_pin_request(IPC* ipc);

__STATIC_INLINE unsigned int lpc_pin_request_inside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    lpc_pin_request(&ipc);
    return ipc.param2;
}

#endif // LPC_PIN_H
