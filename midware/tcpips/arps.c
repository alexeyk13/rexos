/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "arps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "macs.h"
#include "ips.h"

#define ARP_HEADER_SIZE                             (2 + 2 + 1 + 1 + 2)

#define ARP_HRD(buf)                                (((buf)[0] << 8) | (buf)[1])
#define ARP_PRO(buf)                                (((buf)[2] << 8) | (buf)[3])
#define ARP_HLN(buf)                                ((buf)[4])
#define ARP_PLN(buf)                                ((buf)[5])
#define ARP_OP(buf)                                 (((buf)[6] << 8) | (buf)[7])
#define ARP_SHA(buf)                                ((MAC*)((buf) + 8))
#define ARP_SPA(buf)                                ((IP*)((buf) + 8 + 6))
#define ARP_THA(buf)                                ((MAC*)((buf) + 8 + 6 + 4))
#define ARP_TPA(buf)                                ((IP*)((buf) + 8 + 6 + 4 + 6))

#define ARP_CACHE_ITEM(tcpips, i)                    ((ARP_CACHE_ENTRY*)array_at((tcpips)->arp.cache, i))

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
    array_create(&tcpips->arp.cache, sizeof(ARP_CACHE_ENTRY), 1);
}

static void arps_cmd_request(TCPIPS* tcpips, const IP* ip)
{
#if (ARP_DEBUG_FLOW)
    printf("ARP: request to ");
    ip_print(ip);
    printf("\n");
#endif

    const MAC* own_mac;
    IO* io = macs_allocate_io(tcpips);
    if (io == NULL)
        return;
    //hrd
    ((uint8_t*)io_data(io))[0] = (ARP_HRD_ETHERNET >> 8) & 0xff;
    ((uint8_t*)io_data(io))[1] = ARP_HRD_ETHERNET & 0xff;
    //pro
    ((uint8_t*)io_data(io))[2] = (ETHERTYPE_IP >> 8) & 0xff;
    ((uint8_t*)io_data(io))[3] = ETHERTYPE_IP & 0xff;
    //hln
    ((uint8_t*)io_data(io))[4] = sizeof(MAC);
    //pln
    ((uint8_t*)io_data(io))[5] = IP_SIZE;
    //op
    ((uint8_t*)io_data(io))[6] = (ARP_REQUEST >> 8) & 0xff;
    ((uint8_t*)io_data(io))[7] = ARP_REQUEST & 0xff;
    //sha
    own_mac = &tcpips->mac;
    ARP_SHA(((uint8_t*)io_data(io)))->u32.hi = own_mac->u32.hi;
    ARP_SHA(((uint8_t*)io_data(io)))->u32.lo = own_mac->u32.lo;
    //spa
    ARP_SPA(((uint8_t*)io_data(io)))->u32.ip = tcpips->ip.u32.ip;
    //tha
    ARP_THA(((uint8_t*)io_data(io)))->u32.hi = __MAC_REQUEST.u32.hi;
    ARP_THA(((uint8_t*)io_data(io)))->u32.lo = __MAC_REQUEST.u32.lo;
    //tpa
    ARP_TPA(((uint8_t*)io_data(io)))->u32.ip = ip->u32.ip;

    io->data_size = ARP_HEADER_SIZE + sizeof(MAC) * 2 + IP_SIZE * 2;
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

    const MAC* own_mac;
    IO* io = macs_allocate_io(tcpips);
    if (io == NULL)
        return;
    //hrd
    ((uint8_t*)io_data(io))[0] = (ARP_HRD_ETHERNET >> 8) & 0xff;
    ((uint8_t*)io_data(io))[1] = ARP_HRD_ETHERNET & 0xff;
    //pro
    ((uint8_t*)io_data(io))[2] = (ETHERTYPE_IP >> 8) & 0xff;
    ((uint8_t*)io_data(io))[3] = ETHERTYPE_IP & 0xff;
    //hln
    ((uint8_t*)io_data(io))[4] = sizeof(MAC);
    //pln
    ((uint8_t*)io_data(io))[5] = IP_SIZE;
    //op
    ((uint8_t*)io_data(io))[6] = (ARP_REPLY >> 8) & 0xff;
    ((uint8_t*)io_data(io))[7] = ARP_REPLY & 0xff;
    //sha
    own_mac = &tcpips->mac;
    ARP_SHA(((uint8_t*)io_data(io)))->u32.hi = own_mac->u32.hi;
    ARP_SHA(((uint8_t*)io_data(io)))->u32.lo = own_mac->u32.lo;
    //spa
    ARP_SPA(((uint8_t*)io_data(io)))->u32.ip = tcpips->ip.u32.ip;
    //tha
    ARP_THA(((uint8_t*)io_data(io)))->u32.hi = mac->u32.hi;
    ARP_THA(((uint8_t*)io_data(io)))->u32.lo = mac->u32.lo;
    //tpa
    ARP_TPA(((uint8_t*)io_data(io)))->u32.ip = ip->u32.ip;

    io->data_size = ARP_HEADER_SIZE + sizeof(MAC) * 2 + IP_SIZE * 2;
    macs_tx(tcpips, io, mac, ETHERTYPE_ARP);
}

static int arps_index(TCPIPS* tcpips, const IP* ip)
{
    int i;
    for (i = 0; i < array_size(tcpips->arp.cache); ++i)
    {
        if (ARP_CACHE_ITEM(tcpips, i)->ip.u32.ip == ip->u32.ip)
            return i;
    }
    return -1;
}

static void arps_remove(TCPIPS* tcpips, int idx)
{
    IP ip;
    ip.u32.ip = 0;
    if ((ARP_CACHE_ITEM(tcpips, idx)->mac.u32.hi == 0) && (ARP_CACHE_ITEM(tcpips, idx)->mac.u32.lo == 0))
        ip.u32.ip = ARP_CACHE_ITEM(tcpips, idx)->ip.u32.ip;
#if (ARP_DEBUG)
    else
    {
        printf("ARP: route to ");
        ip_print(&ARP_CACHE_ITEM(tcpips, idx)->ip);
        printf(" removed\n");
    }
#endif
    array_remove(&tcpips->arp.cache, idx);

    //inform route on incomplete ARP if not resolved
    if (ip.u32.ip)
        arp_not_resolved(tcpips, &ip);
}

static void arps_insert(TCPIPS* tcpips, const IP* ip, const MAC* mac, unsigned int timeout)
{
    int i, idx;
    unsigned int ttl;
    //don't add dups
    if (arps_index(tcpips, ip) >= 0)
        return;
    //remove first non-static if no place
    if (array_size(tcpips->arp.cache) == ARP_CACHE_SIZE_MAX)
    {
        //first is static, can't remove
        if (ARP_CACHE_ITEM(tcpips, 0)->ttl == 0)
            return;
        arps_remove(tcpips, 0);
    }
    idx = array_size(tcpips->arp.cache);
    ttl = 0;
    //add all static routes to the end of cache
    if (timeout)
    {
        ttl = tcpips_seconds(tcpips) + timeout;
        for (i = 0; i < array_size(tcpips->arp.cache); ++i)
            if ((ARP_CACHE_ITEM(tcpips, i)->ttl > ttl) || (ARP_CACHE_ITEM(tcpips, i)->ttl == 0))
            {
                idx = i;
                break;
            }
    }
    if (array_insert(&tcpips->arp.cache, idx))
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

static void arps_update(TCPIPS* tcpips, const IP* ip, const MAC* mac)
{
    int idx = arps_index(tcpips, ip);
    if (idx < 0)
        return;
    ARP_CACHE_ITEM(tcpips, idx)->mac.u32.hi = mac->u32.hi;
    ARP_CACHE_ITEM(tcpips, idx)->mac.u32.lo = mac->u32.lo;
    ARP_CACHE_ITEM(tcpips, idx)->ttl = tcpips_seconds(tcpips) + ARP_CACHE_TIMEOUT;
#if (ARP_DEBUG)
    printf("ARP: route resolved ");
    ip_print(ip);
    printf(" -> ");
    mac_print(mac);
    printf("\n");
#endif
}

void arps_remove_route(TCPIPS* tcpips, const IP* ip)
{
    int idx = arps_index(tcpips, ip);
    if (idx >= 0)
        arps_remove(tcpips, idx);
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

void arps_link_event(TCPIPS* tcpips, bool link)
{
    if (link)
    {
        //announce IP
        if (tcpips->ip.u32.ip)
            arps_cmd_request(tcpips, &tcpips->ip);
    }
    else
    {
        //flush ARP cache, except static routes
        while (array_size(tcpips->arp.cache) && ARP_CACHE_ITEM(tcpips, 0)->ttl)
            arps_remove(tcpips, 0);
    }
}

void arps_timer(TCPIPS* tcpips, unsigned int seconds)
{
    while (array_size(tcpips->arp.cache) && ARP_CACHE_ITEM(tcpips, 0)->ttl && (ARP_CACHE_ITEM(tcpips, 0)->ttl <= seconds))
        arps_remove(tcpips, 0);
}

bool arps_request(TCPIPS* tcpips, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void arps_rx(TCPIPS* tcpips, IO *io)
{
    if (io->data_size < ARP_HEADER_SIZE)
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    if (ARP_PRO(((uint8_t*)io_data(io))) != ETHERTYPE_IP || ARP_HLN(((uint8_t*)io_data(io))) != sizeof(MAC) || ARP_PLN(((uint8_t*)io_data(io))) != IP_SIZE)
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    switch (ARP_OP(((uint8_t*)io_data(io))))
    {
    case ARP_REQUEST:
        if (ARP_TPA(((uint8_t*)io_data(io)))->u32.ip == tcpips->ip.u32.ip)
        {
            arps_cmd_reply(tcpips, ARP_SHA(((uint8_t*)io_data(io))), ARP_SPA(((uint8_t*)io_data(io))));
            //insert in cache
            arps_insert(tcpips, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))), ARP_CACHE_TIMEOUT);
        }
        //announcment
        if (ARP_TPA(((uint8_t*)io_data(io)))->u32.ip == ARP_SPA(((uint8_t*)io_data(io)))->u32.ip && mac_compare(ARP_THA(((uint8_t*)io_data(io))), &__MAC_REQUEST))
            arps_insert(tcpips, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))), ARP_CACHE_TIMEOUT);
        break;
    case ARP_REPLY:
        if (mac_compare(&tcpips->mac, ARP_THA(((uint8_t*)io_data(io)))))
        {
            arps_update(tcpips, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))));
            arp_resolved(tcpips, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))));
#if (ARP_DEBUG_FLOW)
            printf("ARP: reply from ");
            ip_print(ARP_SPA(((uint8_t*)io_data(io))));
            printf(" is ");
            mac_print(ARP_SHA(((uint8_t*)io_data(io))));
            printf("\n");
#endif
        }
        break;
    }
    tcpips_release_io(tcpips, io);
}

bool arps_resolve(TCPIPS* tcpips, const IP* ip, MAC* mac)
{
    if (arps_lookup(tcpips, ip, mac))
        return true;
    //request mac
    arps_insert(tcpips, ip, mac, ARP_CACHE_INCOMPLETE_TIMEOUT);
    arps_cmd_request(tcpips, ip);
    return false;
}
