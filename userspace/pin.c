/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "pin.h"
#include "sys_config.h"
#include "object.h"
#include "core/core.h"
#include "ipc.h"

#ifdef EXODRIVERS
void pin_enable(unsigned int pin, unsigned int mode, unsigned int mode2)
{
    ipc_post_exo(HAL_CMD(HAL_PIN, IPC_OPEN), pin, mode, mode2);
}

void pin_disable(unsigned int pin)
{
    ipc_post_exo(HAL_CMD(HAL_PIN, IPC_CLOSE), pin, 0, 0);
}
#else

void pin_enable(unsigned int pin, unsigned int mode, unsigned int mode2)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, IPC_OPEN), pin, mode, mode2);
}

void pin_disable(unsigned int pin)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, IPC_CLOSE), pin, 0, 0);
}
#endif //EXODRIVERS
