/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_SDMMC_H
#define LPC_SDMMC_H

#include "lpc_core.h"

typedef struct  {
    int stub;
} SDMMC_DRV;

void lpc_sdmmc_init(CORE* core);
void lpc_sdmmc_request(CORE* core, IPC* ipc);

#endif // LPC_SDMMC_H
