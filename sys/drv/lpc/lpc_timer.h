/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_TIMER_H
#define LPC_TIMER_H

#include "lpc_core.h"

typedef enum {
    TC16B0 = 0,
    TC16B1,
    TC32B0,
    TC32B1,
    TIMER_MAX
} TIMER;

typedef enum {
    LPC_TIMER_START = HAL_IPC(HAL_TIMER),
    LPC_TIMER_STOP
} LPC_TIMER_IPCS;

#define TIMER_FLAG_ENABLE_IRQ                        (1 << 0)
#define TIMER_FLAG_PRIORITY_POS                      16
#define TIMER_FLAG_PRIORITY_MASK                     (0xff << 16)

#define TIMER_CHANNEL_POS                            0
#define TIMER_CHANNEL_MASK                           (3 << 0)

#define TIMER_CHANNEL0                               0
#define TIMER_CHANNEL1                               1
#define TIMER_CHANNEL2                               2
#define TIMER_CHANNEL3                               3

//timer mode. Always relative to channel0 for channels 1-3
#define TIMER_MODE_POS                               2
#define TIMER_MODE_MASK                              (7 << 2)

//value in us units
#define TIMER_MODE_US                                0
//value in hz units
#define TIMER_MODE_HZ                                1
//time in raw clk units
#define TIMER_MODE_CLK                               2
//stop counter after one pulse. Only for channel 0
#define TIMER_MODE_ONE_PULSE                         (1  << 5)

typedef struct {
    unsigned int hpet_start;

} TIMER_DRV;

void lpc_timer_init(CORE* core);
bool lpc_timer_request(CORE* core, IPC* ipc);


#endif // LPC_TIMER_H
