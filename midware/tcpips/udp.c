/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "udp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"

void udp_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    printf("UDP!\n");
    dump(io_data(io), io->data_size);
    ip_release_io(tcpips, io);
}

