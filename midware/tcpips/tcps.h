/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPS_H
#define TCPS_H

#include "../../userspace/io.h"
#include "../../userspace/ip.h"
#include "tcpips.h"

//from tcpip
//TODO: void tcps_init(TCPIPS* tcpips);
//TODO: void tcps_link_changed(TCPIPS* tcpips, bool link);
//TODO: void tcps_request(TCPIPS* tcpips, IPC* ipc);

//from ip
void tcps_rx(TCPIPS* tcpips, IO* io, IP* src);

//from icmp
//TODO: void tcps_icmps_error_process(TCPIPS* tcpips, IO* io, ICMP_ERROR code, const IP* src);


#endif // TCPS_H
