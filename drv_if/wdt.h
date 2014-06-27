/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef WDT_H
#define WDT_H

#include "dev.h"

extern void wdt_enable(WDT_CLASS wdt, int timeout_ms);
extern void wdt_disable(WDT_CLASS wdt);
extern void wdt_kick(WDT_CLASS wdt);

#endif // WDT_H
