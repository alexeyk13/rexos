/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_POWER_H
#define LPC_POWER_H

#include "lpc_exo.h"
#include "../../userspace/ipc.h"
#include "../../userspace/lpc/lpc_driver.h"

typedef struct {
    RESET_REASON reset_reason;
}POWER_DRV;

void lpc_power_init(EXO* exo);
void lpc_power_request(EXO* exo, IPC* ipc);
unsigned int lpc_power_get_clock_inside(POWER_CLOCK_TYPE clock_type);
unsigned int lpc_power_get_core_clock_inside();

#endif // LPC_POWER_H

