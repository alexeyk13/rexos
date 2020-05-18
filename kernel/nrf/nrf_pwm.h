/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/

#ifndef _NRF_PWM_H_
#define _NRF_PWM_H_

#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "nrf_exo.h"
#include "../../userspace/nrf/nrf_driver.h"

typedef struct {
#pragma pack(push,1)
    uint16_t ch_duty[PWM_CHANNELS];
#pragma pack(pop)
} PWM_CHANNEL;


typedef struct {
    uint32_t flags[PWMS_COUNT];
    PWM_CHANNEL seq[PWMS_COUNT];
} PWM_DRV;

void nrf_pwm_init(EXO* exo);
void nrf_pwm_request(EXO* exo, IPC* ipc);

#endif /* _NRF_PWM_H_ */
