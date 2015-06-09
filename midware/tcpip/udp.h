/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UDP_H
#define UDP_H

#include "tcpip.h"
#include "ip.h"
#include "../../userspace/inet.h"
#include "../../userspace/io.h"

//TODO: udp_init

void udp_rx(TCPIP* tcpip, IO* ip, IP* src);


#endif // UDP_H
