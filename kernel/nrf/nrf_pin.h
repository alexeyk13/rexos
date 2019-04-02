/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_PIN_H_
#define _NRF_PIN_H_

#include "nrf_exo.h"
#include "../../userspace/nrf/nrf_driver.h"

typedef struct {
    int* used_pins[GPIO_COUNT];
} GPIO_DRV;

void nrf_pin_init(EXO* exo);
void nrf_pin_request(EXO* exo, IPC* ipc);

#endif /* _NRF_PIN_H_ */
