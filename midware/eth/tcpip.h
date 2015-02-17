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

//allocate block. Find in queue of free blocks, or allocate new. Handle must be checked on return
HANDLE tcpip_allocate_block(TCPIP* tcpip);
//release previously allocated block. Block is not actually freed, just put in queue of free blocks
void tcpip_release_block(TCPIP* tcpip, HANDLE block);
//transmit block. If tx operation is in place, block will be putted to tx queue for later processing
void tcpip_tx(TCPIP* tcpip, HANDLE block, unsigned int size);

#endif // TCPIP_H
