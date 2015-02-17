/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_H
#define TCPIP_H

#include "../../userspace/process.h"
#include "../../userspace/types.h"

typedef struct _TCPIP TCPIP;

extern const REX __TCPIP;

HANDLE tcpip_allocate_block(TCPIP* tcpip);
void tcpip_release_block(TCPIP* tcpip, HANDLE block);
//TODO: tcpip_tx(TCPIP* tcpip, HANDLE block, unsigned int size);

#endif // TCPIP_H
