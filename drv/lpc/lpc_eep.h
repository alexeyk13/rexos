/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_EEP_H
#define LPC_EEP_H

#include "lpc_core.h"

void lpc_eep_init(CORE* core);
void lpc_eep_request(CORE* core, IPC* ipc);

#endif // LPC_EEP_H
