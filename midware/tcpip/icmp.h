/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ICMP_H
#define ICMP_H

#include "tcpip.h"
#include "ip.h"
#include "sys_config.h"
#include "../../userspace/inet.h"

/*
        ICMP header format:

        uint8_t type
        uint8_t code
        uint16_t checksum
        uint16_t id
        uint16_t seq
        uint8_t data[]
 */

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

typedef struct {
    uint16_t id;
#if (ICMP_ECHO)
    uint16_t seq;
    unsigned int seq_count;
    unsigned int success_count;
    IP dst;
    unsigned int ttl;
    HANDLE process;
#endif
} TCPIP_ICMP;

//from tcpip
void icmp_init(TCPIP* tcpip);
bool icmp_request(TCPIP* tcpip, IPC* ipc);
void icmp_timer(TCPIP* tcpip, unsigned int seconds);

//from ip
void icmp_rx(TCPIP* tcpip, IO* io, IP* src);

//tools
#if (ICMP_FLOW_CONTROL)
void icmp_destination_unreachable(TCPIP* tcpip, uint8_t code, IO* original, const IP* dst);
void icmp_time_exceeded(TCPIP* tcpip, uint8_t code, IO* original, const IP* dst);
void icmp_parameter_problem(TCPIP* tcpip, uint8_t offset, IO *original, const IP* dst);
#endif //ICMP_FLOW_CONTROL

#endif // ICMP_H
