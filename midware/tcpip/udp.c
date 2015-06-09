/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "udp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"

void udp_rx(TCPIP* tcpip, IO* io, IP* src)
{
    printf("UDP!\n\r");
    dump(io_data(io), io->data_size);
    ip_release_io(tcpip, io);
}

