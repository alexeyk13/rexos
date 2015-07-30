/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_OTG_H
#define LPC_OTG_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "../../userspace/usb.h"
#include "lpc_config.h"
#if (MONOLITH_USB)
#include "lpc_core.h"
#endif

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
  uint8_t addr;
  bool suspended;
  USB_SPEED speed;
} OTG_DRV;

#if (MONOLITH_USB)
#define SHARED_OTG_DRV                    CORE
#else

typedef struct {
    OTG_DRV otg;
} SHARED_OTG_DRV;

#endif

void lpc_otg_init(SHARED_OTG_DRV* drv);
bool lpc_otg_request(SHARED_OTG_DRV* drv, IPC* ipc);

#endif // LPC_OTG_H