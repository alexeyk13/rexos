/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_TIMER_H_
#define _NRF_TIMER_H_

/*
        timer for nRF
  */

#include "nrf_exo.h"
#include "../../userspace/gpio.h"
#include "../../userspace/htimer.h"

typedef struct {
    //timer specific
    int hpet_uspsc;
} TIMER_DRV;

void nrf_timer_init(EXO* exo);
void nrf_timer_request(EXO* exo, IPC* ipc);


#endif /* _NRF_TIMER_H_ */
