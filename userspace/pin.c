/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#include "pin.h"
#include "sys_config.h"
#include "ipc.h"

void pin_enable(unsigned int pin, unsigned int mode, unsigned int mode2)
{
    ipc_post_exo(HAL_CMD(HAL_PIN, IPC_OPEN), pin, mode, mode2);
}

void pin_disable(unsigned int pin)
{
    ipc_post_exo(HAL_CMD(HAL_PIN, IPC_CLOSE), pin, 0, 0);
}
