/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IPS_H
#define IPS_H

#include "tcpips.h"
#include "../../userspace/ip.h"
#include "../../userspace/ipc.h"
#include "sys_config.h"

typedef struct {
    uint16_t id;
} IPS;

#pragma pack(push, 1)

typedef struct {
    //Version and IHL
    uint8_t ver_ihl;
    //Type of Service
    uint8_t tos;
    //Total length
    uint8_t total_len_be[2];
    //Identification
    uint8_t id[2];
    //Flags, fragment offset
    uint8_t flags_offset_be[2];
    uint8_t ttl;
    uint8_t proto;
    uint8_t header_crc_be[2];
    IP src;
    IP dst;
    //options may follow
} IP_HEADER;

#pragma pack(pop)

typedef struct {
    uint16_t hdr_size;
    uint16_t proto;
} IP_STACK;

//from tcpip
void ips_init(TCPIPS* tcpips);
void ips_open(TCPIPS* tcpips);
bool ips_request(TCPIPS* tcpips, IPC* ipc);

//allocate IP io. If more than (MTU - MAC header - IP header) and fragmentation enabled, will be allocated long frame
IO* ips_allocate_io(TCPIPS* tcpips, unsigned int size, uint8_t proto);
//release previously allocated io. IO is not actually freed, just put in queue of free ios
void ips_release_io(TCPIPS* tcpips, IO* io);
void ips_tx(TCPIPS* tcpips, IO* io, const IP* dst);

//from mac
void ips_rx(TCPIPS* tcpips, IO* io);

#endif // IPS_H
