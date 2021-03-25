/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef POWER_H
#define POWER_H

#include "ipc.h"

typedef enum {
    POWER_MODE_HIGH = 0,
    POWER_MODE_LOW,
    POWER_MODE_ULTRA_LOW,
    POWER_MODE_STOP,
    POWER_MODE_STANDY,
} POWER_MODE;

typedef enum {
    POWER_SET_MODE = IPC_USER,
    POWER_GET_CLOCK,
    POWER_SET_CLOCK,
    POWER_GET_RESET_REASON,
    POWER_SWO_OPEN,
    POWER_MAX
} POWER_IPCS;

typedef enum {
    POWER_CORE_CLOCK = 0,
    POWER_BUS_CLOCK,
    POWER_CLOCK_MAX
} POWER_CLOCK_TYPE;

void power_set_mode(POWER_MODE mode);
unsigned int power_get_clock(POWER_CLOCK_TYPE clock_type);
unsigned int power_get_core_clock();

unsigned int power_set_clock(POWER_CLOCK_TYPE clock_type, uint32_t clock);
unsigned int power_set_core_clock(uint32_t clock);

#endif // POWER_H
