/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef _NRF_RNG_H_
#define _NRF_RNG_H_

#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "nrf_exo.h"


typedef struct {
    HANDLE user;
    IO* io;
    uint8_t last_byte;
} RNG_DRV;

void nrf_rng_init(EXO* exo);
void nrf_rng_request(EXO* exo, IPC* ipc);

#endif /* _NRF_RNG_H_ */
