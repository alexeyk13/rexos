/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TIMER_STM32_H
#define TIMER_STM32_H

/*
        hw delay timer
  */

#include "dev.h"

#define TIMER_FLAG_ONE_PULSE_MODE                (1 << 0)

typedef void (*UTIMER_HANDLER)(TIMER_CLASS);

void timer_enable(TIMER_CLASS timer, UTIMER_HANDLER handler, int priority, unsigned int flags);
void timer_disable(TIMER_CLASS timer);
void timer_start(TIMER_CLASS timer, unsigned int time_us);
void timer_stop(TIMER_CLASS timer);
unsigned int timer_elapsed(TIMER_CLASS timer);
void timer_init_hw();

#endif // TIMER_STM32_H
