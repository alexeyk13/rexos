/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARP_H
#define ARP_H

#include "tcpip.h"
#include "../../userspace/eth.h"
#include "sys_config.h"

#pragma pack(push, 1)

typedef struct {
    uint16_t hrd;
    uint16_t pro;
    uint8_t hln;
    uint8_t pln;
    uint16_t op;
} ARP_HEADER;

#pragma pack(pop)

typedef struct {
    unsigned int stub;
} TCPIP_ARP;
//TODO: arp_init(TCPIP* tcpip);
//TODO: arp_info(TCPIP* tcpip);

void arp_rx(TCPIP* tcpip, MAC* src, void* buf, unsigned int size, HANDLE block);

#endif // ARP_H
