/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ADC_H
#define ADC_H

#include "ipc.h"
#include "cc_macro.h"
#include "object.h"
#include "sys_config.h"

#define ADC_HANDLE_DEVICE                                                      0xffff

#define ADC2uV(raw, vref, res)                                                 ((raw) * (vref) * 100l / (1 << (res)) * 10l)
#define ADC2mV(raw, vref, res)                                                 ((raw) * (vref) / (1 << (res)))

typedef enum {
    ADC_GET = IPC_USER,
    ADC_IPC_MAX
} ADC_IPCS;

__STATIC_INLINE int adc_get(unsigned int channel, unsigned int samplerate)
{
    return get_handle_exo(HAL_REQ(HAL_ADC, ADC_GET), channel, samplerate, 0);
}

__STATIC_INLINE void adc_open()
{
    ipc_post_exo(HAL_REQ(HAL_ADC, IPC_OPEN), ADC_HANDLE_DEVICE, 0, 0);
}

__STATIC_INLINE void adc_open_channel(uint32_t channel)
{
    ipc_post_exo(HAL_REQ(HAL_ADC, IPC_OPEN), channel, 0, 0);
}

#endif // ADC_H
