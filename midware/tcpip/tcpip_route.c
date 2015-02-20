/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpip_route.h"
#include "tcpip_private.h"
#include "tcpip_arp.h"
#include "tcpip_mac.h"

#define ROUTE_QUEUE_ITEM(tcpip, i)                    (((ROUTE_QUEUE_ENTRY*)(array_data((tcpip)->route.tx_queue)))[(i)])
#define ROUTE_QUEUE_ITEMS_COUNT(tcpip)                (array_size((tcpip)->route.tx_queue) / sizeof(ROUTE_QUEUE_ENTRY))

typedef struct {
    TCPIP_IO io;
    IP ip;
} ROUTE_QUEUE_ENTRY;

void tcpip_route_init(TCPIP* tcpip)
{
    array_create(&tcpip->route.tx_queue, sizeof(ROUTE_QUEUE_ENTRY));
}

void tcpip_route_arp_resolved(TCPIP* tcpip, const IP* ip, const MAC* mac)
{
    int i;
    for (i = 0; i < ROUTE_QUEUE_ITEMS_COUNT(tcpip); ++i)
        if (ROUTE_QUEUE_ITEM(tcpip, i).ip.u32.ip == ip->u32.ip)
        {
            //forward to MAC
            tcpip_mac_tx(tcpip, &ROUTE_QUEUE_ITEM(tcpip, i).io, mac, ETHERTYPE_IP);
            array_remove(&tcpip->route.tx_queue, i, sizeof(ROUTE_QUEUE_ENTRY));
        }
}

void tcpip_route_arp_not_resolved(TCPIP* tcpip, const IP* ip)
{
    int i;
    for (i = 0; i < ROUTE_QUEUE_ITEMS_COUNT(tcpip); ++i)
        if (ROUTE_QUEUE_ITEM(tcpip, i).ip.u32.ip == ip->u32.ip)
        {
            //drop if not resolved
            tcpip_release_io(tcpip, &ROUTE_QUEUE_ITEM(tcpip, i).io);
            array_remove(&tcpip->route.tx_queue, i, sizeof(ROUTE_QUEUE_ENTRY));
        }
}

void tcpip_route_tx(TCPIP* tcpip, TCPIP_IO* io, const IP* target)
{
    ROUTE_QUEUE_ENTRY* item;
    //TODO: forward to gateway
    MAC mac;
    if (tcpip_arp_resolve(tcpip, target, &mac))
        tcpip_mac_tx(tcpip, io, &mac, ETHERTYPE_IP);
    else
    {
        //queue before address is resolved
        array_append(&tcpip->route.tx_queue, sizeof(ROUTE_QUEUE_ENTRY));
        item = &ROUTE_QUEUE_ITEM(tcpip, ROUTE_QUEUE_ITEMS_COUNT(tcpip) - 1);
        item->io.block = io->block;
        item->io.buf = io->buf;
        item->io.size = io->size;
        item->ip.u32.ip = target->u32.ip;
    }
}
