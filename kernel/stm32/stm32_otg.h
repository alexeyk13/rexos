/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_OTG_H
#define STM32_OTG_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "stm32_exo.h"

typedef struct {
    IO* io;
    unsigned int size;
    uint16_t mps;
    uint8_t io_active;
} EP;

typedef struct {
  HANDLE device;
  EP* out[USB_EP_COUNT_MAX];
  EP* in[USB_EP_COUNT_MAX];
} USB_DRV;


void stm32_otg_init(EXO* exo);
void stm32_otg_request(EXO* exo, IPC* ipc);

#endif // STM32_OTG_H
