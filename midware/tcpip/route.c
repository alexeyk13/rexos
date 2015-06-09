/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "route.h"
#include "tcpip_private.h"
#include "arp.h"
#include "mac.h"

#define ROUTE_QUEUE_ITEM(tcpip, i)                    ((ROUTE_QUEUE_ENTRY*)array_at((tcpip)->route.tx_queue, i))

typedef struct {
    IO* io;
    IP ip;
} ROUTE_QUEUE_ENTRY;

void route_init(TCPIP* tcpip)
{
    array_create(&tcpip->route.tx_queue, sizeof(ROUTE_QUEUE_ENTRY), 1);
}

bool route_drop(TCPIP* tcpip)
{
    if (array_size(tcpip->route.tx_queue))
    {
        tcpip_release_io(tcpip, ROUTE_QUEUE_ITEM(tcpip, 0)->io);
        array_remove(&tcpip->route.tx_queue, 0);
        return true;
    }
    return false;
}

void arp_resolved(TCPIP* tcpip, const IP* ip, const MAC* mac)
{
    int i;
    for (i = 0; i < array_size(tcpip->route.tx_queue); ++i)
        if (ROUTE_QUEUE_ITEM(tcpip, i)->ip.u32.ip == ip->u32.ip)
        {
            //forward to MAC
            mac_tx(tcpip, ROUTE_QUEUE_ITEM(tcpip, i)->io, mac, ETHERTYPE_IP);
            array_remove(&tcpip->route.tx_queue, i);
        }
}

void arp_not_resolved(TCPIP* tcpip, const IP* ip)
{
    int i;
    for (i = 0; i < array_size(tcpip->route.tx_queue); ++i)
        if (ROUTE_QUEUE_ITEM(tcpip, i)->ip.u32.ip == ip->u32.ip)
        {
            //drop if not resolved
            tcpip_release_io(tcpip, ROUTE_QUEUE_ITEM(tcpip, i)->io);
            array_remove(&tcpip->route.tx_queue, i);
        }
}

void route_tx(TCPIP* tcpip, IO* io, const IP* target)
{
    ROUTE_QUEUE_ENTRY* item;
    //TODO: forward to gateway
    MAC mac;
    if (arp_resolve(tcpip, target, &mac))
        mac_tx(tcpip, io, &mac, ETHERTYPE_IP);
    else
    {
        //queue before address is resolved
        array_append(&tcpip->route.tx_queue);
        item = ROUTE_QUEUE_ITEM(tcpip, array_size(tcpip->route.tx_queue) - 1);
        item->io = io;
        item->ip.u32.ip = target->u32.ip;
    }
}
