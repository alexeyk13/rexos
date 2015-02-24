/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_ICMP_H
#define TCPIP_ICMP_H

#include "tcpip.h"
#include "sys_config.h"
#include "../../userspace/inet.h"

/*
        ICMP header format

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


//TODO: icmp structure, ICMP init


//from ip
void tcpip_icmp_rx(TCPIP* tcpip, TCPIP_IO* io, IP* src);

//TODO: tcpip_icmp_request

#endif // TCPIP_ICMP_H
