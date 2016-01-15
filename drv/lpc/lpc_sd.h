/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_SD_H
#define LPC_SD_H

#include "lpc_core.h"

typedef struct  {
    int stub;
} SD_DRV;

void lpc_sd_init(CORE* core);
void lpc_sd_request(CORE* core, IPC* ipc);

#endif // LPC_SD_H
