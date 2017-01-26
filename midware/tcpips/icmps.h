/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ICMPS_H
#define ICMPS_H

#include "tcpips.h"
#include "ips.h"

#pragma pack(push, 1)

typedef struct {
    uint8_t type;
    uint8_t code;
    uint8_t checksum_be[2];
    uint8_t unused[4];
} ICMP_HEADER;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint8_t checksum_be[2];
    uint8_t param;
    uint8_t unused[3];
} ICMP_HEADER_PARAM;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint8_t checksum_be[2];
    uint8_t id_be[2];
    uint8_t seq_be[2];
} ICMP_HEADER_ID_SEQ;

#pragma pack(pop)

#define ICMP_CMD_ECHO_REPLY                             0
#define ICMP_CMD_DESTINATION_UNREACHABLE                3
#define ICMP_CMD_SOURCE_QUENCH                          4
#define ICMP_CMD_REDIRECT                               5
#define ICMP_CMD_ECHO                                   8
#define ICMP_CMD_TIME_EXCEEDED                          11
#define ICMP_CMD_PARAMETER_PROBLEM                      12
#define ICMP_CMD_TIMESTAMP                              13
#define ICMP_CMD_TIMESTAMP_REPLY                        14
#define ICMP_CMD_INFORMATION_REQUEST                    15
#define ICMP_CMD_INFORMATION_REPLY                      16

#define ICMP_NET_UNREACHABLE                            0
#define ICMP_HOST_UNREACHABLE                           1
#define ICMP_PROTOCOL_UNREACHABLE                       2
#define ICMP_PORT_UNREACHABLE                           3
#define ICMP_FRAGMENTATION_NEEDED_AND_DF_SET            4
#define ICMP_SOURCE_ROUTE_FAILED                        5

#define ICMP_TTL_EXCEED_IN_TRANSIT                      0
#define ICMP_FRAGMENT_REASSEMBLY_EXCEED                 1

typedef enum {
    ICMP_ERROR_NETWORK = ICMP_NET_UNREACHABLE,
    ICMP_ERROR_HOST = ICMP_HOST_UNREACHABLE,
    ICMP_ERROR_PROTOCOL = ICMP_PROTOCOL_UNREACHABLE,
    ICMP_ERROR_PORT = ICMP_PORT_UNREACHABLE,
    ICMP_ERROR_FRAGMENTATION = ICMP_FRAGMENTATION_NEEDED_AND_DF_SET,
    ICMP_ERROR_ROUTE = ICMP_SOURCE_ROUTE_FAILED,
    ICMP_ERROR_PARAMETER = 6,
    ICMP_ERROR_TTL_EXCEED = 7,
    ICMP_ERROR_FRAGMENT_REASSEMBLY_EXCEED = 8,
    ICMP_ERROR_OK = 999
} ICMP_ERROR;

typedef struct {
    uint16_t id, seq;
    IP echo_ip;
    unsigned int ttl;
    HANDLE process;
} ICMPS;

//from tcpip
void icmps_init(TCPIPS* tcpips);
void icmps_request(TCPIPS* tcpips, IPC* ipc);
void icmps_timer(TCPIPS* tcpips, unsigned int seconds);
void icmps_link_changed(TCPIPS* tcpips, bool link);

//from ip
void icmps_rx(TCPIPS* tcpips, IO* io, IP* src);

//flow control
void icmps_tx_error(TCPIPS* tcpips, IO* original, ICMP_ERROR err, unsigned int offset);
void icmps_no_route(TCPIPS* tcpips, IO* original);

#endif // ICMPS_H
