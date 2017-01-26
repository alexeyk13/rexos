/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IPS_H
#define IPS_H

#include "tcpips.h"
#include "../../userspace/ip.h"
#include "../../userspace/ipc.h"
#include "../../userspace/array.h"
#include "sys_config.h"

#define IP_FRAME_MAX_DATA_SIZE                          (TCPIP_MTU - sizeof(IP_HEADER))

typedef struct {
    IP ip;
    uint16_t id;
    bool up;
#if (IP_FIREWALL)
    bool firewall_enabled;
    IP src, mask;
#endif //IP_FIREWALL
#if (IP_FRAGMENTATION)
    unsigned int io_allocated;
    ARRAY* free_io;
    ARRAY* assembly_io;
#endif //IP_FRAGMENTATION
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
    uint8_t id_be[2];
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
#if (IP_FRAGMENTATION)
    bool is_long;
#endif //IP_FRAGMENTATION
} IP_STACK;

//from tcpip
void ips_init(TCPIPS* tcpips);
void ips_request(TCPIPS* tcpips, IPC* ipc);
void ips_link_changed(TCPIPS* tcpips, bool link);
#if (IP_FRAGMENTATION)
void ips_timer(TCPIPS* tcpips, unsigned int seconds);
#endif //IP_FRAGMENTATION

//allocate IP io. If more than (MTU - MAC header - IP header) and fragmentation enabled, will be allocated long frame
IO* ips_allocate_io(TCPIPS* tcpips, unsigned int size, uint8_t proto);
//release previously allocated io. IO is not actually freed, just put in queue of free ios
void ips_release_io(TCPIPS* tcpips, IO* io);
void ips_tx(TCPIPS* tcpips, IO* io, const IP* dst);

//from mac
void ips_rx(TCPIPS* tcpips, IO* io);

#endif // IPS_H
