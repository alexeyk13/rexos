/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/
#ifndef SWO_H
#define SWO_H

#include <stdint.h>
#include "process.h"
#include "ipc.h"

static inline void swo_open()
{
    ipc_post_exo(HAL_CMD(HAL_POWER, POWER_SWO_OPEN), 0, 0, 0);
}

static inline void swo_send_u32(uint8_t channel, uint32_t d)
{
    ITM->PORT[channel].u32 = d;
}

static inline void swo_send_u16(uint8_t channel, uint16_t d)
{
    ITM->PORT[channel].u16 = d;
}

static inline void swo_send_u8(uint8_t channel, uint8_t d)
{
    ITM->PORT[channel].u8 = d;
}

#endif // SWO_H
