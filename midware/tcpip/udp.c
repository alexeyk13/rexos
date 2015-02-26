/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "udp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"

void udp_rx(TCPIP* tcpip, IP_IO* ip_io, IP* src)
{
    printf("UDP!\n\r");
    dump(ip_io->io.buf, ip_io->io.size);
    ip_release_io(tcpip, ip_io);
}

