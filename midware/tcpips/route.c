/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "route.h"
#include "tcpips_private.h"
#include "arp.h"
#include "mac.h"

#define ROUTE_QUEUE_ITEM(tcpips, i)                    ((ROUTE_QUEUE_ENTRY*)array_at((tcpips)->route.tx_queue, i))

typedef struct {
    IO* io;
    IP ip;
} ROUTE_QUEUE_ENTRY;

void route_init(TCPIPS* tcpips)
{
    array_create(&tcpips->route.tx_queue, sizeof(ROUTE_QUEUE_ENTRY), 1);
}

bool route_drop(TCPIPS* tcpips)
{
    if (array_size(tcpips->route.tx_queue))
    {
        tcpips_release_io(tcpips, ROUTE_QUEUE_ITEM(tcpips, 0)->io);
        array_remove(&tcpips->route.tx_queue, 0);
        return true;
    }
    return false;
}

void arp_resolved(TCPIPS* tcpips, const IP* ip, const MAC* mac)
{
    int i;
    for (i = 0; i < array_size(tcpips->route.tx_queue); ++i)
        if (ROUTE_QUEUE_ITEM(tcpips, i)->ip.u32.ip == ip->u32.ip)
        {
            //forward to MAC
            mac_tx(tcpips, ROUTE_QUEUE_ITEM(tcpips, i)->io, mac, ETHERTYPE_IP);
            array_remove(&tcpips->route.tx_queue, i);
        }
}

void arp_not_resolved(TCPIPS* tcpips, const IP* ip)
{
    int i;
    for (i = 0; i < array_size(tcpips->route.tx_queue); ++i)
        if (ROUTE_QUEUE_ITEM(tcpips, i)->ip.u32.ip == ip->u32.ip)
        {
            //drop if not resolved
            tcpips_release_io(tcpips, ROUTE_QUEUE_ITEM(tcpips, i)->io);
            array_remove(&tcpips->route.tx_queue, i);
        }
}

void route_tx(TCPIPS* tcpips, IO* io, const IP* target)
{
    ROUTE_QUEUE_ENTRY* item;
    //TODO: forward to gateway
    MAC mac;
    if (arp_resolve(tcpips, target, &mac))
        mac_tx(tcpips, io, &mac, ETHERTYPE_IP);
    else
    {
        //queue before address is resolved
        array_append(&tcpips->route.tx_queue);
        item = ROUTE_QUEUE_ITEM(tcpips, array_size(tcpips->route.tx_queue) - 1);
        item->io = io;
        item->ip.u32.ip = target->u32.ip;
    }
}
