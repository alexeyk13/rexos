/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpip_route.h"
#include "tcpip_private.h"
#include "tcpip_arp.h"

void tcpip_route_init(TCPIP* tcpip)
{
    //TODO: init queue list
}

void tcpip_route_arp_resolved(TCPIP* tcpip, const IP* ip, const MAC* mac)
{
    //TODO: send all
}

void tcpip_route_arp_not_resolved(TCPIP* tcpip, const IP* ip)
{
    //TODO: flush with target IP
}

void tcpip_route_tx(TCPIP* tcpip, TCPIP_IO* io, const IP* target)
{
    //TODO: lookup ip if not local network
    MAC mac;
    if (tcpip_arp_resolve(tcpip, target, &mac))
        tcpip_mac_tx(tcpip, io, &mac, ETHERTYPE_IP);
    else
    {
        //queue before address is resolved
    }
}
