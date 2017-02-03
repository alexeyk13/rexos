/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_POWER_H
#define TI_POWER_H

#include "ti_exo.h"
#include "../../userspace/power.h"
#include "../../userspace/ipc.h"
#include <stdint.h>

typedef enum {
    POWER_DOMAIN_PERIPH = 0,
    POWER_DOMAIN_SERIAL,
    POWER_DOMAIN_MAX
} POWER_DOMAIN;

typedef struct {
    uint8_t power_domain_used[POWER_DOMAIN_MAX];
} POWER_DRV;

//for internal use only
unsigned int ti_power_get_core_clock();
void ti_power_domain_on(EXO* exo, POWER_DOMAIN domain);
void ti_power_domain_off(EXO* exo, POWER_DOMAIN domain);

void ti_power_init(EXO* exo);
void ti_power_request(EXO* exo, IPC* ipc);

#endif // TI_POWER_H
