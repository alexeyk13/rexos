/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "icmp.h"
#include "tcpip_private.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include <string.h>

#define ICMP_HEADER_SIZE                                            4
#define ICMP_TYPE(buf)                                              ((buf)[0])
#define ICMP_ID(buf)                                                (((buf)[4] << 8) | ((buf)[5]))
#define ICMP_SEQ(buf)                                               (((buf)[6] << 8) | ((buf)[7]))

#if (ICMP_ECHO)
//64 - ip header - mac header - 2 inverted sequence number?!
#define ICMP_DATA_MAGIC_SIZE                                        22
#define ICMP_ECHO_HEADER_SIZE                                       8

static const uint8_t __ICMP_DATA_MAGIC[ICMP_DATA_MAGIC_SIZE] =      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
#endif

void icmp_init(TCPIP* tcpip)
{
    tcpip->icmp.id = 0;
    tcpip->icmp.seq_count = 0;
}

static void icmp_tx(TCPIP* tcpip, IP_IO* ip_io, IP* dst)
{
    ip_io->io.buf[2] = ip_io->io.buf[3] = 0;
    uint16_t cs = ip_checksum(ip_io->io.buf, ip_io->io.size);
    ip_io->io.buf[2] = (cs >> 8) & 0xff;
    ip_io->io.buf[3] = cs & 0xff;
    ip_tx(tcpip, ip_io, dst);
}

#if (ICMP_ECHO_REPLY)
static inline void icmp_cmd_echo(TCPIP* tcpip, IP_IO* ip_io, IP* src)
{
#if (ICMP_DEBUG)
    printf("ICMP: ECHO from ");
    ip_print(src);
    printf("\n\r");
#endif
    ip_io->io.buf[0] = ICMP_CMD_ECHO_REPLY;
    icmp_tx(tcpip, ip_io, src);
}
#endif

#if (ICMP_ECHO)
static bool icmp_cmd_echo_request(TCPIP* tcpip)
{
    IP_IO ip_io;
    if (ip_allocate_io(tcpip, &ip_io, ICMP_DATA_MAGIC_SIZE + ICMP_ECHO_HEADER_SIZE, PROTO_ICMP) == NULL)
        return false;
    ip_io.io.size = ICMP_DATA_MAGIC_SIZE + ICMP_ECHO_HEADER_SIZE;
    //type
    ip_io.io.buf[0] = ICMP_CMD_ECHO;
    //code
    ip_io.io.buf[1] = 0;
    //id
    ip_io.io.buf[4] = (tcpip->icmp.id >> 8) & 0xff;
    ip_io.io.buf[5] = tcpip->icmp.id & 0xff;
    //sequence
    ip_io.io.buf[6] = (tcpip->icmp.seq >> 8) & 0xff;
    ip_io.io.buf[7] = tcpip->icmp.seq & 0xff;
    memcpy(ip_io.io.buf + ICMP_ECHO_HEADER_SIZE, __ICMP_DATA_MAGIC, ICMP_DATA_MAGIC_SIZE);
    tcpip->icmp.ttl = tcpip_seconds(tcpip) + ICMP_ECHO_TIMEOUT;
    icmp_tx(tcpip, &ip_io, &tcpip->icmp.dst);
    return true;
}

static void icmp_echo_next(TCPIP* tcpip)
{
    if (tcpip->icmp.seq < tcpip->icmp.seq_count)
    {
        ++tcpip->icmp.seq;
        if (!icmp_cmd_echo_request(tcpip))
        {
            ipc_post_inline(tcpip->icmp.process, ICMP_PING, tcpip->icmp.dst.u32.ip, tcpip->icmp.success_count, ERROR_OUT_OF_MEMORY);
            tcpip->icmp.seq_count = 0;
        }
    }
    else
    {
        ipc_post_inline(tcpip->icmp.process, ICMP_PING, tcpip->icmp.dst.u32.ip, tcpip->icmp.success_count, tcpip->icmp.seq_count);
        tcpip->icmp.seq_count = 0;
    }
}

static inline void icmp_ping_request(TCPIP* tcpip, const IP* dst, unsigned int count, HANDLE process)
{
    if (tcpip->icmp.seq_count)
    {
        ipc_post_inline(process, ICMP_PING, dst->u32.ip, 0, ERROR_IN_PROGRESS);
        return;
    }
    tcpip->icmp.seq = 0;
    tcpip->icmp.seq_count = count;
    tcpip->icmp.success_count = 0;
    ++tcpip->icmp.id;
    tcpip->icmp.process = process;
    tcpip->icmp.dst.u32.ip = dst->u32.ip;
    icmp_echo_next(tcpip);
}

static inline void icmp_cmd_echo_reply(TCPIP* tcpip, IP_IO* ip_io, IP* src)
{
    bool success = true;
#if (ICMP_DEBUG)
    printf("ICMP: ECHO REPLY from ");
    ip_print(src);
    printf("\n\r");
#endif
    //compare src, sequence, id and data
    if (tcpip->icmp.dst.u32.ip != src->u32.ip)
        success = false;
    if (tcpip->icmp.id != ICMP_ID(ip_io->io.buf) || tcpip->icmp.seq != ICMP_SEQ(ip_io->io.buf))
        success = false;
    if (memcmp(ip_io->io.buf + ICMP_ECHO_HEADER_SIZE, __ICMP_DATA_MAGIC, ICMP_DATA_MAGIC_SIZE))
        success = false;
    if (success)
        ++tcpip->icmp.success_count;
    ip_release_io(tcpip, ip_io);
    icmp_echo_next(tcpip);
}
#endif

bool icmp_request(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    IP ip;
    switch (ipc->cmd)
    {
//TODO: ICMP_INFO
#if (ICMP_ECHO)
    case ICMP_PING:
        ip.u32.ip = ipc->param1;
        icmp_ping_request(tcpip, &ip, ipc->param2, ipc->process);
        break;
#endif
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void icmp_timer(TCPIP* tcpip, unsigned int seconds)
{
#if (ICMP_ECHO)
    if (tcpip->icmp.seq_count && tcpip->icmp.ttl < seconds)
    {
        //ping timeout, send next sequence
        icmp_echo_next(tcpip);
    }
#endif
}

void icmp_rx(TCPIP* tcpip, IP_IO* ip_io, IP* src)
{
    //drop broken ICMP without control, because ICMP is control protocol itself
    if (ip_io->io.size < ICMP_HEADER_SIZE)
    {
        ip_release_io(tcpip, ip_io);
        return;
    }
    if (ip_checksum(ip_io->io.buf, ip_io->io.size))
    {
        ip_release_io(tcpip, ip_io);
        return;
    }

    switch (ICMP_TYPE(ip_io->io.buf))
    {
#if (ICMP_ECHO)
    case ICMP_CMD_ECHO_REPLY:
        icmp_cmd_echo_reply(tcpip, ip_io, src);
        break;
#endif
#if (ICMP_ECHO_REPLY)
    case ICMP_CMD_ECHO:
        icmp_cmd_echo(tcpip, ip_io, src);
        break;
#endif
/*
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
        printf("ICMP: unhandled type %d from ", ICMP_TYPE(ip_io->io.buf));
        ip_print(src);
        printf("\n\r");
#endif
        ip_release_io(tcpip, ip_io);
        break;

    }
}
