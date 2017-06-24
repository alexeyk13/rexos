/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "arps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "../../userspace/error.h"
#include "macs.h"
#include "ips.h"

#define ARP_CACHE_ITEM(tcpips, i)                    ((ARP_CACHE_ENTRY*)array_at((tcpips)->arps.cache, i))

typedef struct {
    IP ip;
    //zero MAC means unresolved yet
    MAC mac;
    //time to live. Zero means static ARP
    unsigned int ttl;
} ARP_CACHE_ENTRY;

static const MAC __MAC_BROADCAST =                  {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
static const MAC __MAC_REQUEST =                    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void arps_init(TCPIPS* tcpips)
{
    array_create(&tcpips->arps.cache, sizeof(ARP_CACHE_ENTRY), 1);
}

static void arps_cmd_request(TCPIPS* tcpips, const IP* ip)
{
#if (ARP_DEBUG_FLOW)
    printf("ARP: request to ");
    ip_print(ip);
    printf("\n");
#endif

    ARP_PACKET* arp;
    IO* io = macs_allocate_io(tcpips);
    if (io == NULL)
        return;
    arp = io_data(io);

    short2be(arp->hrd_be, ARP_HRD_ETHERNET);
    short2be(arp->pro_be, ETHERTYPE_IP);
    arp->hln = sizeof(MAC);
    arp->pln = sizeof(IP);
    short2be(arp->op_be, ARP_REQUEST);
    //sha
    arp->src_mac.u32.hi = tcpips->macs.mac.u32.hi;
    arp->src_mac.u32.lo = tcpips->macs.mac.u32.lo;
    //spa
    arp->src_ip.u32.ip = tcpips->ips.ip.u32.ip;
    //tha
    arp->dst_mac.u32.hi = __MAC_REQUEST.u32.hi;
    arp->dst_mac.u32.lo = __MAC_REQUEST.u32.lo;
    //tpa
    arp->dst_ip.u32.ip = ip->u32.ip;

    io->data_size = sizeof(ARP_PACKET);
    macs_tx(tcpips, io, &__MAC_BROADCAST, ETHERTYPE_ARP);
}

static inline void arps_cmd_reply(TCPIPS* tcpips, MAC* mac, IP* ip)
{
#if (ARP_DEBUG_FLOW)
    printf("ARP: reply to ");
    mac_print(mac);
    printf("(");
    ip_print(ip);
    printf(")\n");
#endif

    ARP_PACKET* arp;
    IO* io = macs_allocate_io(tcpips);
    if (io == NULL)
        return;
    arp = io_data(io);

    short2be(arp->hrd_be, ARP_HRD_ETHERNET);
    short2be(arp->pro_be, ETHERTYPE_IP);
    arp->hln = sizeof(MAC);
    arp->pln = sizeof(IP);
    short2be(arp->op_be, ARP_REPLY);
    //sha
    arp->src_mac.u32.hi = tcpips->macs.mac.u32.hi;
    arp->src_mac.u32.lo = tcpips->macs.mac.u32.lo;
    //spa
    arp->src_ip.u32.ip = tcpips->ips.ip.u32.ip;
    //tha
    arp->dst_mac.u32.hi = mac->u32.hi;
    arp->dst_mac.u32.lo = mac->u32.lo;
    //tpa
    arp->dst_ip.u32.ip = ip->u32.ip;

    io->data_size = sizeof(ARP_PACKET);
    macs_tx(tcpips, io, mac, ETHERTYPE_ARP);
}

static int arps_index(TCPIPS* tcpips, const IP* ip)
{
    int i;
    for (i = 0; i < array_size(tcpips->arps.cache); ++i)
    {
        if (ARP_CACHE_ITEM(tcpips, i)->ip.u32.ip == ip->u32.ip)
            return i;
    }
    return -1;
}

static void arps_remove_item(TCPIPS* tcpips, int idx)
{
    IP ip;
    ip.u32.ip = 0;
    if (mac_compare(&ARP_CACHE_ITEM(tcpips, idx)->mac, &__MAC_REQUEST))
        ip.u32.ip = ARP_CACHE_ITEM(tcpips, idx)->ip.u32.ip;
#if (ARP_DEBUG)
    else
    {
        printf("ARP: route to ");
        ip_print(&ARP_CACHE_ITEM(tcpips, idx)->ip);
        printf(" removed\n");
    }
#endif
    array_remove(&tcpips->arps.cache, idx);

    //inform route on incomplete ARP if not resolved
    if (ip.u32.ip)
        routes_not_resolved(tcpips, &ip);
}

static void arps_insert_item(TCPIPS* tcpips, const IP* ip, const MAC* mac, unsigned int timeout)
{
    int i, idx;
    unsigned int ttl;
    //don't add dups
    if (arps_index(tcpips, ip) >= 0)
        return;
    //remove first non-static if no place
    if (array_size(tcpips->arps.cache) == ARP_CACHE_SIZE_MAX)
    {
        //first is static, can't remove
        if (ARP_CACHE_ITEM(tcpips, 0)->ttl == 0)
            return;
        arps_remove_item(tcpips, 0);
    }
    idx = array_size(tcpips->arps.cache);
    ttl = 0;
    //add all static routes to the end of cache
    if (timeout)
    {
        ttl = tcpips->seconds + timeout;
        for (i = 0; i < array_size(tcpips->arps.cache); ++i)
            if ((ARP_CACHE_ITEM(tcpips, i)->ttl > ttl) || (ARP_CACHE_ITEM(tcpips, i)->ttl == 0))
            {
                idx = i;
                break;
            }
    }
    if (array_insert(&tcpips->arps.cache, idx))
    {
        ARP_CACHE_ITEM(tcpips, idx)->ip.u32.ip = ip->u32.ip;
        ARP_CACHE_ITEM(tcpips, idx)->mac.u32.hi = mac->u32.hi;
        ARP_CACHE_ITEM(tcpips, idx)->mac.u32.lo = mac->u32.lo;
        ARP_CACHE_ITEM(tcpips, idx)->ttl = ttl;
    }
#if (ARP_DEBUG)
    if (mac->u32.hi && mac->u32.lo)
    {
        printf("ARP: route added ");
        ip_print(ip);
        printf(" -> ");
        mac_print(mac);
        printf("\n");
    }
#endif
}

static void arps_update_item(TCPIPS* tcpips, const IP* ip, const MAC* mac)
{
    int idx = arps_index(tcpips, ip);
    if (idx < 0)
        return;
    ARP_CACHE_ITEM(tcpips, idx)->mac.u32.hi = mac->u32.hi;
    ARP_CACHE_ITEM(tcpips, idx)->mac.u32.lo = mac->u32.lo;
    ARP_CACHE_ITEM(tcpips, idx)->ttl = tcpips->seconds + ARP_CACHE_TIMEOUT;
#if (ARP_DEBUG)
    printf("ARP: route resolved ");
    ip_print(ip);
    printf(" -> ");
    mac_print(mac);
    printf("\n");
#endif
}

static bool arps_lookup(TCPIPS* tcpips, const IP* ip, MAC* mac)
{
    int idx = arps_index(tcpips, ip);
    if (idx >= 0)
    {
        mac->u32.hi = ARP_CACHE_ITEM(tcpips, idx)->mac.u32.hi;
        mac->u32.lo = ARP_CACHE_ITEM(tcpips, idx)->mac.u32.lo;
        return true;
    }
    mac->u32.hi = 0;
    mac->u32.lo = 0;
    return false;
}

void arps_link_changed(TCPIPS* tcpips, bool link)
{
    if (link)
    {
        //announce IP
        if (tcpips->ips.ip.u32.ip)
            arps_cmd_request(tcpips, &tcpips->ips.ip);
    }
    else
    {
        //flush ARP cache, except static routes
        while (array_size(tcpips->arps.cache) && ARP_CACHE_ITEM(tcpips, 0)->ttl)
            arps_remove_item(tcpips, 0);
    }
}

void arps_timer(TCPIPS* tcpips, unsigned int seconds)
{
    while (array_size(tcpips->arps.cache) && ARP_CACHE_ITEM(tcpips, 0)->ttl && (ARP_CACHE_ITEM(tcpips, 0)->ttl <= seconds))
        arps_remove_item(tcpips, 0);
}

static inline void arps_add_static(TCPIPS* tcpips, IPC* ipc)
{
    IP ip;
    MAC mac;
    int idx;
    ip.u32.ip = ipc->param1;
    mac.u32.hi = ipc->param2;
    mac.u32.lo = ipc->param3;
    idx = arps_index(tcpips, &ip);
    if (idx >= 0)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    arps_insert_item(tcpips, &ip, &mac, 0);
    ipc->param2 = 0;
}

static inline void arps_remove(TCPIPS* tcpips, IP* ip)
{
    int idx;
    idx = arps_index(tcpips, ip);
    if (idx < 0)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    arps_remove_item(tcpips, idx);
}

static void arps_flush(TCPIPS* tcpips)
{
    while (array_size(tcpips->arps.cache))
        arps_remove_item(tcpips, 0);
}

#if (ARP_DEBUG)
static inline void arps_show_table(TCPIPS* tcpips)
{
    int i;
    ARP_CACHE_ENTRY* arp;
    if (array_size(tcpips->arps.cache) == 0)
    {
        printf("ARP: table is empty\n");
        return;
    }
    printf("       IP             MAC          TTL\n");
    printf("-----------------------------------------\n");
    for (i = 0; i < array_size(tcpips->arps.cache); ++i)
    {
        arp = ARP_CACHE_ITEM(tcpips, i);
        printf("  ");
        ip_print(&arp->ip);
        printf("  ");
        if (mac_compare(&arp->mac, &__MAC_REQUEST))
            printf("REQUESTING ");
        else
            mac_print(&arp->mac);
        printf("  ");
        if (arp->ttl)
            printf("  %d", arp->ttl - tcpips->seconds);
        else
            printf("STATIC");
        printf("\n");
    }
}
#endif //ARP_DEBUG

void arps_request(TCPIPS* tcpips, IPC* ipc)
{
    IP ip;
    switch (HAL_ITEM(ipc->cmd))
    {
    case ARP_ADD_STATIC:
        arps_add_static(tcpips, ipc);
        break;
    case ARP_REMOVE:
        ip.u32.ip = ipc->param1;
        arps_remove(tcpips, &ip);
        break;
    case IPC_FLUSH:
        arps_flush(tcpips);
        break;
#if (ARP_DEBUG)
    case ARP_SHOW_TABLE:
        arps_show_table(tcpips);
        break;
#endif //ARP_DEBUG
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void arps_rx(TCPIPS* tcpips, IO *io)
{
    ARP_PACKET* arp = io_data(io);
    if (io->data_size < sizeof(ARP_PACKET))
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    if (be2short(arp->pro_be) != ETHERTYPE_IP || arp->hln != sizeof(MAC) || arp->pln != sizeof(IP))
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    switch (be2short(arp->op_be))
    {
    case ARP_REQUEST:
        if (arp->dst_ip.u32.ip == tcpips->ips.ip.u32.ip)
        {
            arps_cmd_reply(tcpips, &arp->src_mac, &arp->src_ip);
            //insert in cache
            arps_insert_item(tcpips, &arp->src_ip, &arp->src_mac, ARP_CACHE_TIMEOUT);
        }
        //announcment
        if (arp->dst_ip.u32.ip == arp->src_ip.u32.ip && mac_compare(&arp->dst_mac, &__MAC_REQUEST))
            arps_insert_item(tcpips, &arp->src_ip, &arp->src_mac, ARP_CACHE_TIMEOUT);
        break;
    case ARP_REPLY:
        if (mac_compare(&tcpips->macs.mac, &arp->dst_mac))
        {
            arps_update_item(tcpips, &arp->src_ip, &arp->src_mac);
            routes_resolved(tcpips, &arp->src_ip, &arp->src_mac);
#if (ARP_DEBUG_FLOW)
            printf("ARP: reply from ");
            ip_print(&arp->src_ip);
            printf(" is ");
            mac_print(&arp->src_mac);
            printf("\n");
#endif
        }
        break;
    }
    tcpips_release_io(tcpips, io);
}

bool arps_resolve(TCPIPS* tcpips, const IP* ip, MAC* mac)
{
    if (ip->u32.ip == BROADCAST)
    {
        mac->u32.lo = __MAC_BROADCAST.u32.lo;
        mac->u32.hi = __MAC_BROADCAST.u32.hi;
        return true;
    }
    if (arps_lookup(tcpips, ip, mac))
        return true;
    //request mac
    arps_insert_item(tcpips, ip, &__MAC_REQUEST, ARP_CACHE_INCOMPLETE_TIMEOUT);
    arps_cmd_request(tcpips, ip);
    return false;
}
