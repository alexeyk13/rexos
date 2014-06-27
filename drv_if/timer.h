/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TIMER_H
#define TIMER_H

/*
        hw delay timer
  */

#include "dev.h"

#define TIMER_FLAG_ONE_PULSE_MODE                (1 << 0)

typedef void (*TIMER_HANDLER)(TIMER_CLASS);

extern void timer_enable(TIMER_CLASS timer, TIMER_HANDLER handler, int priority, unsigned int flags);
extern void timer_disable(TIMER_CLASS timer);
extern void timer_start(TIMER_CLASS timer, unsigned int time_us);
extern void timer_stop(TIMER_CLASS timer);
extern unsigned int timer_elapsed(TIMER_CLASS timer);

#endif // TIMER_H
