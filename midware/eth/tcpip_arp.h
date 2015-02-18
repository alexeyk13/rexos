/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_ARP_H
#define TCPIP_ARP_H

#include "tcpip.h"
#include "../../userspace/eth.h"
#include "../../userspace/inet.h"
#include "../../userspace/array.h"
#include <stdint.h>
#include "sys_config.h"

/*
    ARP packet structure

    uint16_t hrd
    uint16_t pro
    uint8_t hln
    uint16_t pln
    uint16_t op
    uint8_t sha[]
    uint8_t spa[]
    uint8_t tha[]
    uint8_t tpa[]
 */

#define ARP_HRD_ETHERNET                1
#define ARP_HRD_IEEE802                 6

#define ARP_REQUEST                     1
#define ARP_REPLY                       2
#define RARP_REQUEST                    3
#define RARP_REPLY                      4

typedef struct {
    IP ip;
    MAC mac;
    unsigned int ttl;
} ARP_CACHE_ENTRY;

#define ARP_CACHE_STATIC                0
#define ARP_CACHE_INCOMPLETE            ((unsigned int)-1)

typedef struct {
    unsigned int stub;
} TCPIP_ARP;

//from tcpip
void tcpip_arp_init(TCPIP* tcpip);
//from mac
void tcpip_arp_rx(TCPIP* tcpip, TCPIP_IO* io);

//from ip level. If NULL returned, sender must queue request for asynchronous answer
const MAC* tcpip_arp_resolve(TCPIP* tcpip, const IP* ip);

#endif // TCPIP_ARP_H
