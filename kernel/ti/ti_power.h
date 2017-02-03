/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_POWER_H
#define TI_POWER_H

#include "ti_exo.h"
#include "../../userspace/power.h"
#include "../../userspace/ipc.h"

//for internal use only
unsigned int ti_power_get_core_clock();

void ti_power_init(EXO* exo);
void ti_power_request(EXO* exo, IPC* ipc);

#endif // TI_POWER_H
