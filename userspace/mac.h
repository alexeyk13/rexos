/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MAC_H
#define MAC_H

#include <stdint.h>
#include <stdbool.h>
#include "ipc.h"

#define MAC_INDIVIDUAL_ADDRESS                      (0 << 0)
#define MAC_MULTICAST_ADDRESS                       (1 << 0)
#define MAC_BROADCAST_ADDRESS                       (3 << 0)

#define MAC_CAST_MASK                               (3 << 0)

#define ETHERTYPE_IP                                0x0800
#define ETHERTYPE_ARP                               0x0806
#define ETHERTYPE_IPV6                              0x86dd

#pragma pack(push, 1)

typedef union {
    uint8_t u8[6];
    struct {
        uint32_t hi;
        uint16_t lo;
    }u32;
} MAC;

typedef struct {
    MAC dst;
    MAC src;
    uint8_t lentype_be[2];
} MAC_HEADER;

#pragma pack(pop)

typedef enum {
    MAC_ENABLE_FIREWALL = IPC_USER,
    MAC_DISABLE_FIREWALL
}MAC_IPCS;

void mac_print(const MAC* mac);
bool mac_compare(const MAC* src, const MAC* dst);
void mac_enable_firewall(HANDLE tcpip, const MAC* src);
void mac_disable_firewall(HANDLE tcpip);

#endif // MAC_H
