/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_POWER_H
#define STM32_POWER_H

#include "stm32_core.h"
#include "../../userspace/stm32_driver.h"

typedef struct {
    int write_count;
#if (STM32_DECODE_RESET)
    RESET_REASON reset_reason;
#endif //STM32_DECODE_RESET
    int dma_count[DMA_COUNT];
}POWER_DRV;

void stm32_power_init(CORE* core);
bool stm32_power_request(CORE* core, IPC* ipc);

__STATIC_INLINE unsigned int stm32_power_request_inside(CORE* core, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    stm32_power_request(core, &ipc);
    return ipc.param2;
}

__STATIC_INLINE unsigned int stm32_power_get_clock_outside(void* unused, STM32_POWER_CLOCKS type)
{
    return stm32_core_request_outside(unused, STM32_POWER_GET_CLOCK, type, 0, 0);
}

__STATIC_INLINE unsigned int stm32_power_get_clock_inside(CORE* core, STM32_POWER_CLOCKS type)
{
    IPC ipc;
    ipc.cmd = STM32_POWER_GET_CLOCK;
    ipc.param1 = (unsigned int)type;
    stm32_power_request(core, &ipc);
    return ipc.param2;
}

#endif // STM32_POWER_H
