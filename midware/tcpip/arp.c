/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "arp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include "mac.h"
#include "ip.h"

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

#define ARP_CACHE_ITEM(tcpip, i)                    ((ARP_CACHE_ENTRY*)array_at((tcpip)->arp.cache, i))

typedef struct {
    IP ip;
    //zero MAC means unresolved yet
    MAC mac;
    //time to live. Zero means static ARP
    unsigned int ttl;
} ARP_CACHE_ENTRY;

static const MAC __MAC_BROADCAST =                  {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
static const MAC __MAC_REQUEST =                    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void arp_init(TCPIP* tcpip)
{
    array_create(&tcpip->arp.cache, sizeof(ARP_CACHE_ENTRY), 1);
}

static void arp_cmd_request(TCPIP* tcpip, const IP* ip)
{
#if (ARP_DEBUG_FLOW)
    printf("ARP: request to ");
    ip_print(ip);
    printf("\n");
#endif

    const MAC* own_mac;
    IO* io = mac_allocate_io(tcpip);
    if (io == NULL)
        return;
    //hrd
    ((uint8_t*)io_data(io))[0] = (ARP_HRD_ETHERNET >> 8) & 0xff;
    ((uint8_t*)io_data(io))[1] = ARP_HRD_ETHERNET & 0xff;
    //pro
    ((uint8_t*)io_data(io))[2] = (ETHERTYPE_IP >> 8) & 0xff;
    ((uint8_t*)io_data(io))[3] = ETHERTYPE_IP & 0xff;
    //hln
    ((uint8_t*)io_data(io))[4] = MAC_SIZE;
    //pln
    ((uint8_t*)io_data(io))[5] = IP_SIZE;
    //op
    ((uint8_t*)io_data(io))[6] = (ARP_REQUEST >> 8) & 0xff;
    ((uint8_t*)io_data(io))[7] = ARP_REQUEST & 0xff;
    //sha
    own_mac = tcpip_mac(tcpip);
    ARP_SHA(((uint8_t*)io_data(io)))->u32.hi = own_mac->u32.hi;
    ARP_SHA(((uint8_t*)io_data(io)))->u32.lo = own_mac->u32.lo;
    //spa
    ARP_SPA(((uint8_t*)io_data(io)))->u32.ip = tcpip_ip(tcpip)->u32.ip;
    //tha
    ARP_THA(((uint8_t*)io_data(io)))->u32.hi = __MAC_REQUEST.u32.hi;
    ARP_THA(((uint8_t*)io_data(io)))->u32.lo = __MAC_REQUEST.u32.lo;
    //tpa
    ARP_TPA(((uint8_t*)io_data(io)))->u32.ip = ip->u32.ip;

    io->data_size = ARP_HEADER_SIZE + MAC_SIZE * 2 + IP_SIZE * 2;
    mac_tx(tcpip, io, &__MAC_BROADCAST, ETHERTYPE_ARP);
}

static inline void arp_cmd_reply(TCPIP* tcpip, MAC* mac, IP* ip)
{
#if (ARP_DEBUG_FLOW)
    printf("ARP: reply to ");
    mac_print(mac);
    printf("(");
    ip_print(ip);
    printf(")\n");
#endif

    const MAC* own_mac;
    IO* io = mac_allocate_io(tcpip);
    if (io == NULL)
        return;
    //hrd
    ((uint8_t*)io_data(io))[0] = (ARP_HRD_ETHERNET >> 8) & 0xff;
    ((uint8_t*)io_data(io))[1] = ARP_HRD_ETHERNET & 0xff;
    //pro
    ((uint8_t*)io_data(io))[2] = (ETHERTYPE_IP >> 8) & 0xff;
    ((uint8_t*)io_data(io))[3] = ETHERTYPE_IP & 0xff;
    //hln
    ((uint8_t*)io_data(io))[4] = MAC_SIZE;
    //pln
    ((uint8_t*)io_data(io))[5] = IP_SIZE;
    //op
    ((uint8_t*)io_data(io))[6] = (ARP_REPLY >> 8) & 0xff;
    ((uint8_t*)io_data(io))[7] = ARP_REPLY & 0xff;
    //sha
    own_mac = tcpip_mac(tcpip);
    ARP_SHA(((uint8_t*)io_data(io)))->u32.hi = own_mac->u32.hi;
    ARP_SHA(((uint8_t*)io_data(io)))->u32.lo = own_mac->u32.lo;
    //spa
    ARP_SPA(((uint8_t*)io_data(io)))->u32.ip = tcpip_ip(tcpip)->u32.ip;
    //tha
    ARP_THA(((uint8_t*)io_data(io)))->u32.hi = mac->u32.hi;
    ARP_THA(((uint8_t*)io_data(io)))->u32.lo = mac->u32.lo;
    //tpa
    ARP_TPA(((uint8_t*)io_data(io)))->u32.ip = ip->u32.ip;

    io->data_size = ARP_HEADER_SIZE + MAC_SIZE * 2 + IP_SIZE * 2;
    mac_tx(tcpip, io, mac, ETHERTYPE_ARP);
}

static int arp_index(TCPIP* tcpip, const IP* ip)
{
    int i;
    for (i = 0; i < array_size(tcpip->arp.cache); ++i)
    {
        if (ARP_CACHE_ITEM(tcpip, i)->ip.u32.ip == ip->u32.ip)
            return i;
    }
    return -1;
}

static void arp_remove(TCPIP* tcpip, int idx)
{
    IP ip;
    ip.u32.ip = 0;
    if ((ARP_CACHE_ITEM(tcpip, idx)->mac.u32.hi == 0) && (ARP_CACHE_ITEM(tcpip, idx)->mac.u32.lo == 0))
        ip.u32.ip = ARP_CACHE_ITEM(tcpip, idx)->ip.u32.ip;
#if (ARP_DEBUG)
    else
    {
        printf("ARP: route to ");
        ip_print(&ARP_CACHE_ITEM(tcpip, idx)->ip);
        printf(" removed\n");
    }
#endif
    array_remove(&tcpip->arp.cache, idx);

    //inform route on incomplete ARP if not resolved
    if (ip.u32.ip)
        arp_not_resolved(tcpip, &ip);
}

static void arp_insert(TCPIP* tcpip, const IP* ip, const MAC* mac, unsigned int timeout)
{
    int i, idx;
    unsigned int ttl;
    //don't add dups
    if (arp_index(tcpip, ip) >= 0)
        return;
    //remove first non-static if no place
    if (array_size(tcpip->arp.cache) == ARP_CACHE_SIZE_MAX)
    {
        //first is static, can't remove
        if (ARP_CACHE_ITEM(tcpip, 0)->ttl == 0)
            return;
        arp_remove(tcpip, 0);
    }
    idx = array_size(tcpip->arp.cache);
    ttl = 0;
    //add all static routes to the end of cache
    if (timeout)
    {
        ttl = tcpip_seconds(tcpip) + timeout;
        for (i = 0; i < array_size(tcpip->arp.cache); ++i)
            if ((ARP_CACHE_ITEM(tcpip, i)->ttl > ttl) || (ARP_CACHE_ITEM(tcpip, i)->ttl == 0))
            {
                idx = i;
                break;
            }
    }
    if (array_insert(&tcpip->arp.cache, idx))
    {
        ARP_CACHE_ITEM(tcpip, idx)->ip.u32.ip = ip->u32.ip;
        ARP_CACHE_ITEM(tcpip, idx)->mac.u32.hi = mac->u32.hi;
        ARP_CACHE_ITEM(tcpip, idx)->mac.u32.lo = mac->u32.lo;
        ARP_CACHE_ITEM(tcpip, idx)->ttl = ttl;
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

static void arp_update(TCPIP* tcpip, const IP* ip, const MAC* mac)
{
    int idx = arp_index(tcpip, ip);
    if (idx < 0)
        return;
    ARP_CACHE_ITEM(tcpip, idx)->mac.u32.hi = mac->u32.hi;
    ARP_CACHE_ITEM(tcpip, idx)->mac.u32.lo = mac->u32.lo;
    ARP_CACHE_ITEM(tcpip, idx)->ttl = tcpip_seconds(tcpip) + ARP_CACHE_TIMEOUT;
#if (ARP_DEBUG)
    printf("ARP: route resolved ");
    ip_print(ip);
    printf(" -> ");
    mac_print(mac);
    printf("\n");
#endif
}

void arp_remove_route(TCPIP* tcpip, const IP* ip)
{
    int idx = arp_index(tcpip, ip);
    if (idx >= 0)
        arp_remove(tcpip, idx);
}

static bool arp_lookup(TCPIP* tcpip, const IP* ip, MAC* mac)
{
    int idx = arp_index(tcpip, ip);
    if (idx >= 0)
    {
        mac->u32.hi = ARP_CACHE_ITEM(tcpip, idx)->mac.u32.hi;
        mac->u32.lo = ARP_CACHE_ITEM(tcpip, idx)->mac.u32.lo;
        return true;
    }
    mac->u32.hi = 0;
    mac->u32.lo = 0;
    return false;
}

void arp_link_event(TCPIP* tcpip, bool link)
{
    if (link)
    {
        //announce IP
        if (tcpip_ip(tcpip)->u32.ip)
            arp_cmd_request(tcpip, tcpip_ip(tcpip));
    }
    else
    {
        //flush ARP cache, except static routes
        while (array_size(tcpip->arp.cache) && ARP_CACHE_ITEM(tcpip, 0)->ttl)
            arp_remove(tcpip, 0);
    }
}

void arp_timer(TCPIP* tcpip, unsigned int seconds)
{
    while (array_size(tcpip->arp.cache) && ARP_CACHE_ITEM(tcpip, 0)->ttl && (ARP_CACHE_ITEM(tcpip, 0)->ttl <= seconds))
        arp_remove(tcpip, 0);
}

bool arp_request(TCPIP* tcpip, IPC* ipc)
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

void arp_rx(TCPIP* tcpip, IO *io)
{
    if (io->data_size < ARP_HEADER_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    if (ARP_PRO(((uint8_t*)io_data(io))) != ETHERTYPE_IP || ARP_HLN(((uint8_t*)io_data(io))) != MAC_SIZE || ARP_PLN(((uint8_t*)io_data(io))) != IP_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    switch (ARP_OP(((uint8_t*)io_data(io))))
    {
    case ARP_REQUEST:
        if (ARP_TPA(((uint8_t*)io_data(io)))->u32.ip == tcpip_ip(tcpip)->u32.ip)
        {
            arp_cmd_reply(tcpip, ARP_SHA(((uint8_t*)io_data(io))), ARP_SPA(((uint8_t*)io_data(io))));
            //insert in cache
            arp_insert(tcpip, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))), ARP_CACHE_TIMEOUT);
        }
        //announcment
        if (ARP_TPA(((uint8_t*)io_data(io)))->u32.ip == ARP_SPA(((uint8_t*)io_data(io)))->u32.ip && mac_compare(ARP_THA(((uint8_t*)io_data(io))), &__MAC_REQUEST))
            arp_insert(tcpip, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))), ARP_CACHE_TIMEOUT);
        break;
    case ARP_REPLY:
        if (mac_compare(tcpip_mac(tcpip), ARP_THA(((uint8_t*)io_data(io)))))
        {
            arp_update(tcpip, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))));
            arp_resolved(tcpip, ARP_SPA(((uint8_t*)io_data(io))), ARP_SHA(((uint8_t*)io_data(io))));
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
    tcpip_release_io(tcpip, io);
}

bool arp_resolve(TCPIP* tcpip, const IP* ip, MAC* mac)
{
    if (arp_lookup(tcpip, ip, mac))
        return true;
    //request mac
    arp_insert(tcpip, ip, mac, ARP_CACHE_INCOMPLETE_TIMEOUT);
    arp_cmd_request(tcpip, ip);
    return false;
}
