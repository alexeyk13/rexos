/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_SDMMC_H
#define LPC_SDMMC_H

#include "lpc_core.h"
#include "../sdmmcs.h"
#include "../../userspace/io.h"

typedef enum {
    SDMMC_STATE_IDLE,
    SDMMC_STATE_RX,
    SDMMC_STATE_TX
} SDMMC_STATE;

typedef struct _LPC_SDMMC_DESCR LPC_SDMMC_DESCR;

typedef struct  {
    SDMMCS sdmmcs;
    SDMMC_STATE state;
    struct _LPC_SDMMC_DESCR* descr;
    IO* io;
    HANDLE process;
    unsigned int count;
    bool active;
} SDMMC_DRV;

void lpc_sdmmc_init(CORE* core);
void lpc_sdmmc_request(CORE* core, IPC* ipc);

#endif // LPC_SDMMC_H
