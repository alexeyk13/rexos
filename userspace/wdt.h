/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
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
