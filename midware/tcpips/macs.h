/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MACS_H
#define MACS_H

#include "tcpips.h"
#include "../../userspace/mac.h"
#include "sys_config.h"
#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    MAC dst;
    MAC src;
    uint8_t lentype_be[2];
} MAC_HEADER;

#pragma pack(pop)

//from tcpip process
void macs_init(TCPIPS* tcpips);
void macs_open(TCPIPS* tcpips);
bool macs_request(TCPIPS* tcpips, IPC* ipc);
void macs_rx(TCPIPS* tcpips, IO* io);

IO* macs_allocate_io(TCPIPS* tcpips);
void macs_tx(TCPIPS* tcpips, IO* io, const MAC* dst, uint16_t lentype);

#endif // MACS_H
