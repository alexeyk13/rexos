/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_POWER_H
#define LPC_POWER_H

#include "lpc_core.h"
#include "../../userspace/ipc.h"
#include "../../userspace/lpc/lpc_driver.h"

void lpc_power_init(CORE* core);
void lpc_power_request(CORE* core, IPC* ipc);

typedef struct {
    RESET_REASON reset_reason;
}POWER_DRV;

__STATIC_INLINE unsigned int lpc_power_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    lpc_power_request(core, &ipc);
    return ipc.param2;
}

__STATIC_INLINE unsigned int lpc_power_get_core_clock_outside(void* unused)
{
    return lpc_core_request_outside(unused, HAL_REQ(HAL_POWER, LPC_POWER_GET_CORE_CLOCK), 0, 0, 0);
}

__STATIC_INLINE unsigned int lpc_power_get_core_clock_inside(CORE* core)
{
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_POWER, LPC_POWER_GET_CORE_CLOCK);
    lpc_power_request(core, &ipc);
    return ipc.param2;
}


#endif // LPC_POWER_H

