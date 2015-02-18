/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpip_ip.h"
#include "tcpip_private.h"
#include "sys_config.h"
#include "../../userspace/stdio.h"

#if (SYS_INFO) || (TCPIP_DEBUG)
void print_ip(IP* ip)
{
    int i;
    for (i = 0; i < IP_SIZE; ++i)
    {
        printf("%d", ip->u8[i]);
        if (i < IP_SIZE - 1)
            printf(".");
    }
}
#endif //(SYS_INFO) || (TCPIP_DEBUG)

const IP* tcpip_ip(TCPIP* tcpip)
{
    return &tcpip->ip.ip;
}

void tcpip_ip_init(TCPIP* tcpip)
{
    tcpip->ip.ip.u32.ip = IP_MAKE(0, 0, 0, 0);
}

#if (SYS_INFO)
static inline void tcpip_ip_info(TCPIP* tcpip)
{
    printf("IP info\n\r");
    printf("IP: ");
    print_ip(&tcpip->ip.ip);
    printf("\n\r");
}
#endif

static inline void tcpip_ip_set(TCPIP* tcpip, uint32_t ip)
{
    tcpip->ip.ip.u32.ip = ip;
}

static inline void tcpip_ip_get(TCPIP* tcpip, HANDLE process)
{
    ipc_post_inline(process, IP_GET, 0, tcpip->ip.ip.u32.ip, 0);
}

bool tcpip_ip_request(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        tcpip_ip_info(tcpip);
        need_post = true;
        break;
#endif
    case IP_SET:
        tcpip_ip_set(tcpip, ipc->param1);
        need_post = true;
        break;
    case IP_GET:
        tcpip_ip_get(tcpip, ipc->process);
        break;
    default:
        break;
    }
    return need_post;
}

void tcpip_ip_rx(TCPIP* tcpip, TCPIP_IO* io)
{
    printf("got ip request\n\r");
    tcpip_release_io(tcpip, io);
}
