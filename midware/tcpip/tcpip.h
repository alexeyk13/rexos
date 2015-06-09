/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_H
#define TCPIP_H

#include "../../userspace/process.h"
#include "../../userspace/types.h"
#include "../../userspace/io.h"

typedef struct _TCPIP TCPIP;

extern const REX __TCPIP;

//allocate io. Find in queue of free io, or allocate new. Handle must be checked on return
IO* tcpip_allocate_io(TCPIP* tcpip);
//release previously allocated block. Block is not actually freed, just put in queue of free blocks
void tcpip_release_io(TCPIP* tcpip, IO* io);
//transmit. If tx operation is in place (2 tx for double buffering), io will be putted in queue for later processing
void tcpip_tx(TCPIP* tcpip, IO* io);
//get seconds from start
unsigned int tcpip_seconds(TCPIP* tcpip);


#endif // TCPIP_H
