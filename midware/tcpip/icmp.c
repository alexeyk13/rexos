/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "icmp.h"
#include "tcpip_private.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"

void icmp_rx(TCPIP* tcpip, TCPIP_IO* io, IP* src)
{
    printf("ICMP!\n\r");
    dump(io->buf, io->size);
    ip_release_io(tcpip, io);
}
