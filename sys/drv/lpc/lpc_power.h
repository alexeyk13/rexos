/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_POWER_H
#define LPC_POWER_H

#include "lpc_core.h"

typedef enum {
    LPC_POWER_GET_SYSTEM_CLOCK = HAL_IPC(HAL_POWER),
    LPC_POWER_UPDATE_CLOCK,
    LPC_POWER_GET_RESET_REASON
} LPC_POWER_IPCS;

typedef enum {
    RESET_REASON_POWERON = 0,
    RESET_REASON_EXTERNAL,
    RESET_REASON_WATCHDOG,
    RESET_REASON_BROWNOUT,
    RESET_REASON_SOFTWARE,
    RESET_REASON_UNKNOWN
} RESET_REASON;

typedef struct {
    RESET_REASON reset_reason;
}POWER_DRV;

void lpc_power_init(CORE* core);
bool lpc_power_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int lpc_power_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    lpc_power_request(core, &ipc);
    return ipc.param1;
}

__STATIC_INLINE unsigned int lpc_power_get_system_clock_outside(void* unused)
{
    return lpc_core_request_outside(unused, LPC_POWER_GET_SYSTEM_CLOCK, 0, 0, 0);
}

__STATIC_INLINE unsigned int lpc_power_get_system_clock_inside(CORE* core)
{
    IPC ipc;
    ipc.cmd = LPC_POWER_GET_SYSTEM_CLOCK;
    lpc_power_request(core, &ipc);
    return ipc.param1;
}


#endif // LPC_POWER_H

