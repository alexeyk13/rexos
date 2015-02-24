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
//TODO: icmp timer

//from ip
void icmp_rx(TCPIP* tcpip, IP_IO* ip_io, IP* src);

#endif // ICMP_H
