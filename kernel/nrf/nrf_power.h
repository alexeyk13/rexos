/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_POWER_H_
#define _NRF_POWER_H_

#include "nrf_exo.h"
#include "../../userspace/nrf/nrf.h"
#include "../../userspace/nrf/nrf_driver.h"
#include "../../userspace/power.h"

typedef struct {
#if (NRF_DECODE_RESET)
    RESET_REASON reset_reason;
#endif //STM32_DECODE_RESET
} POWER_DRV;

void nrf_power_init(EXO* exo);
void nrf_power_request(EXO* exo, IPC* ipc);
int get_core_clock_internal();

__STATIC_INLINE unsigned int nrf_power_get_clock_inside(EXO* exo)
{
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_POWER, POWER_GET_CLOCK);
    nrf_power_request(exo, &ipc);
    return ipc.param2;
}


#endif /* _NRF_POWER_H_ */
