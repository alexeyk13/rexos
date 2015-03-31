/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_DAC_H
#define STM32_DAC_H

#include "../../userspace/sys.h"
#include "../../userspace/stm32_driver.h"
#include "../../userspace/dac.h"
#include "stm32_config.h"
#include "stm32_core.h"
#include <stdint.h>

#if defined (STM32F1)
#if (DAC_DUAL_CHANNEL)
#define DAC_CHANNELS_COUNT_USER                     1
#define DAC_MANY                                    0
#else
#define DAC_CHANNELS_COUNT_USER                     2
#define DAC_MANY                                    1
#endif
#else
#define DAC_CHANNELS_COUNT_USER                     1
#define DAC_MANY                                    0
#endif

typedef enum {
    STM32_DAC_UNDERFLOW_DEBUG = DAC_IPC_MAX
} STM32_DAC_IPCS;

typedef struct {
#if (DAC_STREAM)
    HANDLE block, process;
    void* ptr;
    uint16_t cnt, half;
    uint16_t size;
#endif
    unsigned int samplerate;
    void* fifo;
    DAC_MODE mode;
    bool active;
} DAC_CHANNEL;

typedef struct {
    DAC_CHANNEL channels[DAC_CHANNELS_COUNT_USER];
} DAC_DRV;

bool stm32_dac_request(CORE* core, IPC* ipc);
void stm32_dac_init(CORE* core);

#endif // STM32_DAC_H
