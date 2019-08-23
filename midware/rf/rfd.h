/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _RF_RFD_H_
#define _RF_RFD_H_

#include "../../userspace/nrf/nrf_driver.h"
#include "../../userspace/nrf/nrf_radio_config.h"
#include "../../userspace/core/core.h"

typedef struct {
    RADIO_MODE mode;
    uint8_t mac_addr[6];
} RFD;

#endif /* _RF_RFD_H_ */
