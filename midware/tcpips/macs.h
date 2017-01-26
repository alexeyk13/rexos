/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MACS_H
#define MACS_H

#include "tcpips.h"
#include "../../userspace/mac.h"
#include "sys_config.h"
#include <stdint.h>

typedef struct {
    MAC mac;
#if (MAC_FIREWALL)
    MAC src;
    bool firewall_enabled;
#endif //MAC_FIREWALL
} MACS;

//from tcpip process
void macs_init(TCPIPS* tcpips);
void macs_open(TCPIPS* tcpips);
void macs_request(TCPIPS* tcpips, IPC* ipc);
void macs_link_changed(TCPIPS* tcpips, bool link);
void macs_rx(TCPIPS* tcpips, IO* io);

IO* macs_allocate_io(TCPIPS* tcpips);
void macs_tx(TCPIPS* tcpips, IO* io, const MAC* dst, uint16_t lentype);

#endif // MACS_H
