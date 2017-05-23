/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_USB_H
#define LPC_USB_H

#include "../../userspace/process.h"
#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "lpc_config.h"
#include "lpc_exo.h"
#include <stdbool.h>

typedef struct {
    IO* io;
    void* fifo;
    unsigned int size;
    uint16_t mps;
    uint8_t io_active;
} EP;

typedef struct {
  HANDLE device;
  EP* out[USB_EP_COUNT_MAX];
  EP* in[USB_EP_COUNT_MAX];
  uint8_t addr;
  unsigned int pending_ipc;
  bool device_ready, device_pending;
} USB_DRV;

void lpc_usb_init(EXO* exo);
void lpc_usb_request(EXO* exo, IPC* ipc);

#endif // LPC_USB_H
