/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_RF_H
#define TI_RF_H

#include "ti_exo.h"
#include "../../userspace/ipc.h"
#include <stdbool.h>

typedef struct {
    bool active;
} RF_DRV;

void ti_rf_init(EXO* exo);
void ti_rf_request(EXO* exo, IPC* ipc);


#endif // TI_RF_H
