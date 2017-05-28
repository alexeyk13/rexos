/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_EEP_H
#define LPC_EEP_H

#include "lpc_exo.h"

void lpc_eep_init(EXO* exo);
void lpc_eep_request(EXO* exo, IPC* ipc);

#endif // LPC_EEP_H
