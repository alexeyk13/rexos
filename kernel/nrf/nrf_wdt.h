/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef _NRF_WDT_H_
#define _NRF_WDT_H_

#include "../../userspace/sys.h"
#include "../../userspace/ipc.h"

void nrf_wdt_pre_init();
void nrf_wdt_init();

void nrf_wdt_request(IPC* ipc);

#endif /* _NRF_WDT_H_ */
