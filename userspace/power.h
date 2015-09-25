/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef POWER_H
#define POWER_H

#include "ipc.h"
#include "cc_macro.h"
#include "sys_config.h"

typedef enum {
    POWER_MODE_HIGH = 0,
    POWER_MODE_LOW,
    POWER_MODE_ULTRA_LOW,
    POWER_MODE_STOP,
    POWER_MODE_STANDY
} POWER_MODE;

typedef enum {
    POWER_SET_MODE = IPC_USER,
    POWER_MAX
} POWER_IPCS;

__STATIC_INLINE void power_set_mode(POWER_MODE mode)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_POWER, POWER_SET_MODE), mode, 0, 0);
}

#endif // POWER_H
