/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARP_H
#define ARP_H

#include "tcpip.h"
#include "../../userspace/eth.h"
#include "../../userspace/inet.h"
#include "../../userspace/array.h"
#include "../../userspace/ipc.h"
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
    ARRAY* cache;
} TCPIP_ARP;

//from tcpip
void arp_init(TCPIP* tcpip);
void arp_link_event(TCPIP* tcpip, bool link);
void arp_timer(TCPIP* tcpip, unsigned int seconds);
bool arp_request(TCPIP* tcpip, IPC* ipc);

//from mac
void arp_rx(TCPIP* tcpip, TCPIP_IO* io);

//from route. If false returned, sender must queue request for asynchronous answer
bool arp_resolve(TCPIP* tcpip, const IP* ip, MAC* mac);

#endif // TCPIP_ARP_H
