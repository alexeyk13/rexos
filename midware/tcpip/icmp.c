/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "icmp.h"
#include "ip.h"
#include "tcpip_private.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"

#define ICMP_HEADER_SIZE                4
#define ICMP_TYPE(buf)                  ((buf)[0])

static inline void icmp_cmd_echo(tcpip, io, src)
{

}

void icmp_rx(TCPIP* tcpip, TCPIP_IO* io, IP* src)
{
    //drop broken ICMP without control, cause ICMP is control protocol itself
    if (io->size < ICMP_HEADER_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    if (io->size < ICMP_HEADER_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }

    switch (ICMP_TYPE(io->buf))
    {
    case ICMP_CMD_ECHO:
        icmp_cmd_echo(tcpip, io, src);
        break;
/*    case ICMP_CMD_ECHO_REPLY:
    case ICMP_CMD_DESTINATION_UNREACHABLE:
    case ICMP_CMD_SOURCE_QUENCH:
    case ICMP_CMD_REDIRECT:
    case ICMP_CMD_TIME_EXCEEDED:
    case ICMP_CMD_PARAMETER_PROBLEM:
    case ICMP_CMD_TIMESTAMP:
    case ICMP_CMD_TIMESTAMP_REPLY:
    case ICMP_CMD_INFORMATION_REQUEST:
    case ICMP_CMD_INFORMATION_REPLY:*/
    default:
#if (ICMP_DEBUG)
        printf("ICMP: unhandled type %d from ", ICMP_TYPE(io->buf));
        ip_print(src);
        printf("\n\r");
#endif
        ip_release_io(tcpip, io);
        break;

    }
}
