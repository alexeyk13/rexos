/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARPS_H
#define ARPS_H

#include "tcpips.h"
#include "../../userspace/eth.h"
#include "../../userspace/array.h"
#include "../../userspace/ipc.h"
#include "../../userspace/arp.h"
#include <stdint.h>
#include "sys_config.h"

#pragma pack(push, 1)

typedef struct {
    uint8_t hrd_be[2];
    uint8_t pro_be[2];
    uint8_t hln;
    uint8_t pln;
    uint8_t op_be[2];
    MAC src_mac;
    IP src_ip;
    MAC dst_mac;
    IP dst_ip;
} ARP_PACKET;

#pragma pack(pop)

#define ARP_HRD_ETHERNET                1
#define ARP_HRD_IEEE802                 6

#define ARP_REQUEST                     1
#define ARP_REPLY                       2
#define RARP_REQUEST                    3
#define RARP_REPLY                      4

typedef struct {
    ARRAY* cache;
} ARPS;

//from tcpip
void arps_init(TCPIPS* tcpips);
void arps_link_changed(TCPIPS* tcpips, bool link);
void arps_timer(TCPIPS* tcpips, unsigned int seconds);
void arps_request(TCPIPS* tcpips, IPC* ipc);

//from mac
void arps_rx(TCPIPS* tcpips, IO* io);

//from route. If false returned, sender must queue request for asynchronous answer
bool arps_resolve(TCPIPS* tcpips, const IP* ip, MAC* mac);

#endif // ARPS_H
