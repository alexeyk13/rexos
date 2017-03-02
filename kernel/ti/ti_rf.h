/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_RF_H
#define TI_RF_H

#include "ti_exo.h"
#include "../../userspace/ipc.h"
#include "../../userspace/process.h"
#include "../../userspace/array.h"
#include <stdbool.h>

typedef enum {
    RF_STATE_IDLE,
    RF_STATE_CONFIGURED,
    RF_STATE_READY
} RF_STATE;

typedef struct {
    HANDLE heap;
    void* cur;
    RF_STATE state;
    uint16_t cmd;
    HANDLE process;
} RF_DRV;

void ti_rf_init(EXO* exo);
void ti_rf_request(EXO* exo, IPC* ipc);


#endif // TI_RF_H
