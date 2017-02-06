/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_TIMER_H
#define TI_TIMER_H

#include "ti_exo.h"
#include "../../userspace/ipc.h"

typedef struct {
    //cached value
    unsigned int core_clock, core_clock_us;
} TIMER_DRV;

void ti_timer_init(EXO* exo);
void ti_timer_request(EXO* exo, IPC* ipc);


#endif // TI_TIMER_H
