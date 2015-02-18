/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MAC_H
#define MAC_H

#include "tcpip.h"
#include "../../userspace/eth.h"
#include "sys_config.h"
#include <stdint.h>

typedef struct {
    MAC mac;
} TCPIP_MAC;

/*
    MAC packet header format:

        MAC             dst
        MAC             src
        uint16_t        type/len
 */
#define MAC_HEADER_SIZE                             (MAC_SIZE * 2 + 2)
#define MAC_HEADER_LENTYPE_OFFSET                   (MAC_SIZE * 2)

#define MAC_INDIVIDUAL_ADDRESS                      (0 << 0)
#define MAC_MULTICAST_ADDRESS                       (1 << 0)
#define MAC_BROADCAST_ADDRESS                       (3 << 0)

#define MAC_CAST_MASK                               (3 << 0)

#define ETHERTYPE_IP                                0x0800
#define ETHERTYPE_ARP                               0x0806
#define ETHERTYPE_IPV6                              0x86dd

void tcpip_mac_init(TCPIP* tcpip);

#if (SYS_INFO) || (TCPIP_DEBUG)
void print_mac(MAC* mac);
#endif //(SYS_INFO) || (TCPIP_DEBUG)

#if (SYS_INFO)
void mac_info(TCPIP* tcpip);
#endif

//from tcpip process
void tcpip_mac_rx(TCPIP* tcpip, uint8_t* buf, unsigned int size, HANDLE block);

unsigned int mac_prepare_tx(TCPIP* tcpip, uint8_t* raw, const MAC* dst, uint16_t lentype);
bool mac_compare(MAC* src, MAC* dst);

const MAC* tcpip_mac(TCPIP* tcpip);

#endif // MAC_H
