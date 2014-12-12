/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_USB_H
#define LPC_USB_H

#include "../../../userspace/process.h"
#include "../../../userspace/ipc.h"
#include "lpc_config.h"
#include "sys_config.h"
#if (MONOLITH_USB)
#include "lpc_core.h"
#endif

typedef struct {
    HANDLE block;
    void* ptr;
    void* fifo;
    unsigned int size, processed;
    HANDLE process;
    uint16_t mps;
    uint8_t io_active;
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


void lpc_usb_init(SHARED_USB_DRV* drv);
bool lpc_usb_request(SHARED_USB_DRV* drv, IPC* ipc);

#if !(MONOLITH_USB)
extern const REX __LPC_USB;
#endif

#endif // LPC_USB_H
