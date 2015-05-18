/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HTIMER_H
#define HTIMER_H

#include "sys.h"
#include "file.h"
#include "cc_macro.h"
#include "sys_config.h"

typedef enum {
    TIMER_START = HAL_IPC(HAL_TIMER),
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
    TIMER_CHANNEL_OUTPUT_PWM_RISE,
    TIMER_CHANNEL_OUTPUT_PWM_FALL,
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

__STATIC_INLINE int htimer_open(int num, unsigned int flags)
{
    return fopen(object_get(SYS_OBJ_CORE), HAL_HANDLE(HAL_TIMER, num), flags);
}

__STATIC_INLINE int htimer_close(int num)
{
    return fclose(object_get(SYS_OBJ_CORE), HAL_HANDLE(HAL_TIMER, num));
}

__STATIC_INLINE void htimer_start(int num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    ack(object_get(SYS_OBJ_CORE), TIMER_START, HAL_HANDLE(HAL_TIMER, num), value_type, value);
}

__STATIC_INLINE void htimer_stop(int num)
{
    ack(object_get(SYS_OBJ_CORE), TIMER_STOP, HAL_HANDLE(HAL_TIMER, num), 0, 0);
}

__STATIC_INLINE void htimer_setup_channel(int num, int channel, TIMER_CHANNEL_TYPE type, unsigned int value)
{
    ack(object_get(SYS_OBJ_CORE), TIMER_SETUP_CHANNEL, HAL_HANDLE(HAL_TIMER, num), (channel << TIMER_CHANNEL_POS) | (type << TIMER_CHANNEL_TYPE_POS), value);
}

#endif // HTIMER_H
