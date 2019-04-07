/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef _NRF_RF_H_
#define _NRF_RF_H_

#include "../../userspace/ipc.h"
#include "sys_config.h"
#include "nrf_exo.h"
#include <stdbool.h>

#define MAX_PDU_SIZE    40

typedef struct {
    //
    uint8_t pdu[40];
} RADIO_DRV;

void nrf_rf_init(EXO* exo);
void nrf_rf_request(EXO* exo, IPC* ipc);


#endif /* _NRF_RF_H_ */
