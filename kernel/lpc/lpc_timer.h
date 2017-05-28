/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_TIMER_H
#define LPC_TIMER_H

#include "lpc_exo.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "../../userspace/htimer.h"

typedef struct {
    unsigned int hpet_start;
    uint8_t main_channel[TIMER_MAX];
} TIMER_DRV;

void lpc_timer_init(EXO* exo);
bool lpc_timer_request(EXO* exo, IPC* ipc);

//for power profile switching
void lpc_timer_suspend(EXO* exo);
void lpc_timer_adjust(EXO* exo);

#endif // LPC_TIMER_H
