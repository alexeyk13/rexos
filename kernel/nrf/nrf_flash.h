/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef _NRF_FLASH_H_
#define _NRF_FLASH_H_

#include "nrf_exo.h"
#include "../../userspace/nrf/nrf.h"
#include "../../userspace/nrf/nrf_driver.h"
#include "../../userspace/flash.h"

typedef struct {
    bool active;
    HANDLE user, activity;
    unsigned int offset;
    uint8_t* page;
} FLASH_DRV;

void nrf_flash_init(EXO* exo);
void nrf_flash_request(EXO* exo, IPC* ipc);

#endif /* _NRF_FLASH_H_ */
