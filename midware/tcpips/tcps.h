/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPS_H
#define TCPS_H

#include "../../userspace/io.h"
#include "../../userspace/ip.h"
#include "../../userspace/so.h"
#include "tcpips.h"
#include "icmps.h"

#define TCP_FLAG_FIN                                (1 << 0)
#define TCP_FLAG_SYN                                (1 << 1)
#define TCP_FLAG_RST                                (1 << 2)
#define TCP_FLAG_PSH                                (1 << 3)
#define TCP_FLAG_ACK                                (1 << 4)
#define TCP_FLAG_URG                                (1 << 5)
#define TCP_FLAG_MSK                                0x3f
#define TCP_FLAGS_COUNT                             6

#define TCP_OPTS_END                                0
#define TCP_OPTS_NOOP                               1
#define TCP_OPTS_MSS                                2

typedef struct {
    SO listen, tcbs;
    uint16_t dynamic;
} TCPS;


//from tcpip
void tcps_init(TCPIPS* tcpips);
void tcps_link_changed(TCPIPS* tcpips, bool link);
void tcps_request(TCPIPS* tcpips, IPC* ipc);

//from ip
void tcps_rx(TCPIPS* tcpips, IO* io, IP* src);

//from icmp
void tcps_icmps_error_process(TCPIPS* tcpips, IO* io, ICMP_ERROR code, const IP* src);

#endif // TCPS_H
