/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_USBL_H
#define STM32_USBL_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"
#include "stm32_config.h"
#include "sys_config.h"
#if (MONOLITH_USB)
#include "stm32_core.h"
#endif

typedef struct {
    HANDLE block;
    void* ptr;
    unsigned int size, processed;
    uint16_t mps;
    uint8_t io_active;
    HANDLE process;
} EP;

typedef struct {
  HANDLE device;
  EP* out[USB_EP_COUNT_MAX];
  EP* in[USB_EP_COUNT_MAX];
  uint8_t addr;
} USB_DRV;

#if (MONOLITH_USB)
#define SHARED_USB_DRV                    CORE
#else

typedef struct {
    USB_DRV usb;
} SHARED_USB_DRV;

#endif


void stm32_usb_init(SHARED_USB_DRV* drv);
bool stm32_usb_request(SHARED_USB_DRV* drv, IPC* ipc);

#if !(MONOLITH_USB)
extern const REX __STM32_USBL;
#endif

#endif // STM32_USBL_H
