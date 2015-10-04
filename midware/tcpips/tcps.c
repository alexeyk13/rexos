/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"

void tcps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    printf("TCP!!\n");
    dump(io_data(io), io->data_size);
    ips_release_io(tcpips, io);
}
