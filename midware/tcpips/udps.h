/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UDP_H
#define UDP_H

#include "tcpips.h"
#include "ips.h"
#include "../../userspace/ip.h"
#include "../../userspace/io.h"

//TODO: udp_init

void udp_rx(TCPIPS* tcpips, IO* ip, IP* src);


#endif // UDP_H
