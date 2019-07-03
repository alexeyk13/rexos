/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_ADC_H_
#define _NRF_ADC_H_

#include "../../userspace/ipc.h"
#include "nrf_exo.h"

typedef struct {
    bool active;
} ADC_DRV;

void nrf_adc_init(EXO* exo);
void nrf_adc_request(EXO* exo, IPC* ipc);


#endif /* _NRF_ADC_H_ */
