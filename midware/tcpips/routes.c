/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "routes.h"
#include "tcpips_private.h"
#include "arps.h"
#include "macs.h"
#include "icmps.h"

#define ROUTE_QUEUE_ITEM(tcpips, i)                    ((ROUTE_QUEUE_ENTRY*)array_at((tcpips)->routes.tx_queue, i))

typedef struct {
    IO* io;
    IP ip;
} ROUTE_QUEUE_ENTRY;

void routes_init(TCPIPS* tcpips)
{
    array_create(&tcpips->routes.tx_queue, sizeof(ROUTE_QUEUE_ENTRY), 1);
}

bool routes_drop(TCPIPS* tcpips)
{
    if (array_size(tcpips->routes.tx_queue))
    {
        tcpips_release_io(tcpips, ROUTE_QUEUE_ITEM(tcpips, 0)->io);
        array_remove(&tcpips->routes.tx_queue, 0);
        return true;
    }
    return false;
}

void routes_link_changed(TCPIPS* tcpips, bool link)
{
    if (!link)
    {
        //drop all routes
        while (routes_drop(tcpips)) {}
    }
}

void routes_resolved(TCPIPS* tcpips, const IP* ip, const MAC* mac)
{
    int i;
    for (i = 0; i < array_size(tcpips->routes.tx_queue); ++i)
        if (ROUTE_QUEUE_ITEM(tcpips, i)->ip.u32.ip == ip->u32.ip)
        {
            //forward to MAC
            macs_tx(tcpips, ROUTE_QUEUE_ITEM(tcpips, i)->io, mac, ETHERTYPE_IP);
            array_remove(&tcpips->routes.tx_queue, i);
        }
}

void routes_not_resolved(TCPIPS* tcpips, const IP* ip)
{
    int i;
    for (i = 0; i < array_size(tcpips->routes.tx_queue); ++i)
        if (ROUTE_QUEUE_ITEM(tcpips, i)->ip.u32.ip == ip->u32.ip)
        {
#if (ICMP)
            icmps_no_route(tcpips, ROUTE_QUEUE_ITEM(tcpips, i)->io);
#endif //ICMP
            //drop if not resolved
            tcpips_release_io(tcpips, ROUTE_QUEUE_ITEM(tcpips, i)->io);
            array_remove(&tcpips->routes.tx_queue, i);
        }
}

void routes_tx(TCPIPS* tcpips, IO* io, const IP* target)
{
    ROUTE_QUEUE_ENTRY* item;
    //for gateway support forward should be declared here
    MAC mac;
    if (arps_resolve(tcpips, target, &mac))
        macs_tx(tcpips, io, &mac, ETHERTYPE_IP);
    else
    {
        //queue before address is resolved
        array_append(&tcpips->routes.tx_queue);
        item = ROUTE_QUEUE_ITEM(tcpips, array_size(tcpips->routes.tx_queue) - 1);
        item->io = io;
        item->ip.u32.ip = target->u32.ip;
    }
}
