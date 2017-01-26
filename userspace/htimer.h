/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HTIMER_H
#define HTIMER_H

#include <stdbool.h>
#include "ipc.h"

typedef enum {
    TIMER_START = IPC_USER,
    TIMER_STOP,
    TIMER_SETUP_CHANNEL
} TIMER_IPCS;

//bit 0. Ext clock
#define TIMER_EXT_CLOCK                         (1 << 0)
//bit 1 one pulse mode
#define TIMER_ONE_PULSE                         (1 << 1)
//bit 2 IRQ enabled
#define TIMER_IRQ_ENABLE                        (1 << 2)
//bits 3..7 reserved
//bits 8..15 IRQ priority
#define TIMER_IRQ_PRIORITY_POS                  8
#define TIMER_IRQ_PRIORITY_MASK                 (0xff << 8)
#define TIMER_IRQ_PRIORITY_VALUE(flags)         (((flags) >> 8) & 0xff)
//bits 16.. are hardware specific

typedef enum {
    TIMER_VALUE_HZ = 0,
    TIMER_VALUE_US,
    TIMER_VALUE_CLK
} TIMER_VALUE_TYPE;

//not all modes may be supported by hardware
typedef enum {
    TIMER_CHANNEL_GENERAL = 0,
    TIMER_CHANNEL_INPUT_RISING,
    TIMER_CHANNEL_INPUT_FALLING,
    TIMER_CHANNEL_INPUT_RISING_FALLING,
    TIMER_CHANNEL_PWM,
    TIMER_CHANNEL_DISABLE
} TIMER_CHANNEL_TYPE;

//bits 0..7 channel
#define TIMER_CHANNEL_POS                       0
#define TIMER_CHANNEL_MASK                      (0xff << 0)
#define TIMER_CHANNEL_VALUE(raw)                (((raw) >> 0) & 0xff)
//bits 8..15
#define TIMER_CHANNEL_TYPE_POS                  8
#define TIMER_CHANNEL_TYPE_MASK                 (0xff << 8)
#define TIMER_CHANNEL_TYPE_VALUE(raw)           (((raw) >> 8) & 0xff)
//bits 16..31 user specific

/** \addtogroup HTIMER HTIMER
    hardware timer

    \{
 */

/**
    \brief open hardware timer
    \param num: number, hardware specific
    \param flags: timer flags. Not all may be supported by hardware
    \retval none
*/
bool htimer_open(int num, unsigned int flags);

/**
    \brief close hardware timer
    \param num: number, hardware specific
    \retval none
*/
void htimer_close(int num);

/**
    \brief start hardware timer
    \param num: number, hardware specific
    \param value_type: value type: bus clocks, us, hz
    \param value: value int value_type units
    \retval none
*/
void htimer_start(int num, TIMER_VALUE_TYPE value_type, unsigned int value);

/**
    \brief stop hardware timer
    \param num: number, hardware specific
    \retval none
*/
void htimer_stop(int num);

/**
    \brief setup hardware timer channel.
    \details maybe not supported for all or some hardware timers.
    \param num: number, hardware specific
    \param channel: number of timer channel
    \param type: channel mode: generic, PWM, input signal counter, etc
    \param value: match value, pwm impulse wide, etc. Generally in clocks units
    \retval none
*/
void htimer_setup_channel(int num, int channel, TIMER_CHANNEL_TYPE type, unsigned int value);

/** \} */ // end of htimer group

#endif // HTIMER_H
