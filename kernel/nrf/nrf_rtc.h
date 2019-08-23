/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_RTC_H_
#define _NRF_RTC_H_

#include "nrf_exo.h"
#include "../../userspace/gpio.h"
#include "../../userspace/ipc.h"
#include "../../userspace/nrf/nrf_driver.h"

////bit 0. Ext clock
//#define RTC_EXT_CLOCK                         (1 << 0)
////bit 1 one pulse mode
//#define RTC_ONE_PULSE                         (1 << 1)
////bit 2 IRQ enabled
#define RTC_IRQ_ENABLE                        (1 << 2)
//bits 3..7 reserved
//bits 8..15 IRQ priority
#define RTC_IRQ_PRIORITY_POS                  8
#define RTC_IRQ_PRIORITY_MASK                 (0xff << 8)
#define RTC_IRQ_PRIORITY_VALUE(flags)         (((flags) >> 8) & 0xff)
//bits 16.. are hardware specific

typedef enum {
    RTC_CC_TYPE_SEC = 0,
    RTC_CC_TYPE_CLK,
    RTC_CC_DISABLE
} RTC_CC_TYPE;

typedef struct {
    //cached value
    unsigned int clock_freq;
} RTC_DRV;

void nrf_rtc_init(EXO* exo);
void nrf_rtc_request(EXO* exo, IPC* ipc);

#endif /* REXOS_KERNEL_NRF_NRF_RTC_H_ */
