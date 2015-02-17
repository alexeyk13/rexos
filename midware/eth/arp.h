/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARP_H
#define ARP_H

#include "tcpip.h"
#include "../../userspace/eth.h"
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
    unsigned int stub;
} TCPIP_ARP;
//TODO: arp_init(TCPIP* tcpip);
//TODO: arp_info(TCPIP* tcpip);

void arp_rx(TCPIP* tcpip, uint8_t* buf, unsigned int size, HANDLE block);

#endif // ARP_H
