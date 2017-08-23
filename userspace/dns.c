/*
 RExOS - embedded RTOS
 Copyright (c) 2011-2017, Alexey Kramarenko
 All rights reserved.
 */

#include "dns.h"
#include "io.h"
#include <string.h>

void dns_set_name(HANDLE tcpip, const char* domain_name)
{
    IO* io;
    io = io_create(64);
    memcpy(io_data(io), domain_name, 63);
    ack(tcpip, HAL_REQ(HAL_DNS, IPC_WRITE), tcpip, (uint32_t)io, 0);
    io_destroy(io);
}
