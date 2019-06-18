/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DNSS_H
#define DNSS_H

#include "config.h"
#include "tcpips.h"
#include "stdlib.h"
#include "stdio.h"
#include "endian.h"
#include <string.h>

#define DNS_PORT               53
#define DNS_MAX_NAME_LEN       64

#define DNS_TYPE_A              1
#define DNS_TYPE_PTR            12
#define DNS_CLASS_IN            1

#define DNS_ERROR_OK            0
#define DNS_ERROR_FORMAT        1
#define DNS_ERROR_FAILURE       2
#define DNS_ERROR_NOTEXIST      3
#define DNS_ERROR_NOT_IMPLEMENT 4
#define DNS_ERROR_REFUSED       5

#define DNS_FLAG_RESPONSE       (1 << 15)
#define DNS_FLAG_AA             (1 << 10)
#define DNS_FLAG_TRUNCATE       (1 << 9)
#define DNS_FLAG_REQ_DESIRED    (1 << 8)
#define DNS_FLAG_REQ_AVALIBLE   (1 << 7)

#pragma pack(push,1)
typedef struct {
    uint8_t id_be[2];
    uint16_t flags;
    uint16_t qdcount; // number of entries in the question section
    uint16_t ancount; // number of resource records in the answer section
    uint16_t nscount; // number of name server resource records in the authority records section.
    uint16_t arcount; // number of resource records in the additional records section.
    uint8_t name[0];
} DNS_HEADER;

typedef struct {
    uint16_t compress;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t len;
    uint32_t ip;
} DNS_ANSWER;
#pragma pack(pop)

typedef struct {
    char name[DNS_MAX_NAME_LEN];
    char name_www[DNS_MAX_NAME_LEN];
} _DNSS;

void dnss_init(TCPIPS* tcpips);
bool dnss_rx(TCPIPS* tcpips, IO* io, IP* src);
void dnss_request(TCPIPS* tcpips, IPC* ipc);

#endif //DNSS_H
