/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIPS_H
#define TCPIPS_H

#include "../../userspace/process.h"
#include "../../userspace/types.h"
#include "../../userspace/io.h"

typedef struct _TCPIPS TCPIPS;

//allocate io. Find in queue of free io, or allocate new. Handle must be checked on return
IO* tcpips_allocate_io(TCPIPS* tcpips);
//release previously allocated io. Io is not actually freed, just put in queue of free ios
void tcpips_release_io(TCPIPS* tcpips, IO* io);
//transmit. If tx operation is in place (2 tx for double buffering), io will be putted in queue for later processing
void tcpips_tx(TCPIPS* tcpips, IO* io);

#endif // TCPIPS_H
