/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef WDT_H
#define WDT_H

#include "ipc.h"

typedef enum {
    WDT_KICK = IPC_USER,

    WDT_HAL_MAX
} WDT_IPCS;

void wdt_kick();

#endif // WDT_H
