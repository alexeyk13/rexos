/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ADC_H
#define ADC_H

#include "sys.h"
#include "ipc.h"
#include "cc_macro.h"
#include "sys_config.h"

#define ADC2uV(raw, vref, res)                                                 ((raw) * (vref) * 100 / (1 << (res)) * 10ul)
#define ADC2mV(raw, vref, res)                                                 ((raw) * (vref) / (1 << (res)))

typedef enum {
    ADC_GET = IPC_USER,
    ADC_IPC_MAX
} ADC_IPCS;

__STATIC_INLINE int adc_get(int channel)
{
    return get_handle(object_get(SYS_OBJ_ADC), HAL_REQ(HAL_ADC, ADC_GET), channel, 0, 0);
}

#endif // ADC_H
