/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_OTG_H
#define LPC_OTG_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "../../userspace/usb.h"
#include "lpc_config.h"
#include "lpc_exo.h"
#include "sys_config.h"

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
  unsigned int pending_ipc;
  bool device_ready, device_pending;
} OTG_TYPE;

typedef struct {
  OTG_TYPE* otg[USB_COUNT];
} OTG_DRV;

void lpc_otg_init(EXO* exo);
void lpc_otg_request(EXO* exo, IPC* ipc);

#endif // LPC_OTG_H
