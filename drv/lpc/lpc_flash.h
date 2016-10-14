/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_FLASH_H
#define LPC_FLASH_H

#include "lpc_core.h"
#include <stdbool.h>
#include "../../userspace/types.h"

typedef struct {
    bool active;
    HANDLE user, activity;
} FLASH_DRV;

void lpc_flash_init(CORE* core);
void lpc_flash_request(CORE* core, IPC* ipc);

#endif // LPC_FLASH_H
