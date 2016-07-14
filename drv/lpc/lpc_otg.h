/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_OTG_H
#define LPC_OTG_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "../../userspace/usb.h"
#include "lpc_config.h"
#include "lpc_core.h"

typedef struct {
    IO* io;
} EP;

typedef struct {
  HANDLE device;
  EP* out[USB_EP_COUNT_MAX];
  EP* in[USB_EP_COUNT_MAX];
  unsigned int read_size[USB_EP_COUNT_MAX];
  bool suspended;
  USB_SPEED speed;
} OTG_TYPE;

typedef struct {
  OTG_TYPE* otg[USB_COUNT];
} OTG_DRV;

void lpc_otg_init(CORE* core);
void lpc_otg_request(CORE* core, IPC* ipc);

#endif // LPC_OTG_H
