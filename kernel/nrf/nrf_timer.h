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
#include "../../userspace/nrf/nrf_driver.h"
#if(NRF_PWM_DRIVER)
#include "nrf_pwm.h"
#endif // NRF_PWM_DRIVER

typedef struct {
    unsigned int flags[TIMER_MAX];
    //cached value
    unsigned int hpet_start;
    unsigned int core_clock, hpet_uspsc, psc;
#if(NRF_PWM_DRIVER)
    PWM_DRV pwm;
#endif // NRF_PWM_DRIVER

} TIMER_DRV;

void nrf_timer_init(EXO* exo);
void nrf_timer_request(EXO* exo, IPC* ipc);


#endif /* _NRF_TIMER_H_ */
