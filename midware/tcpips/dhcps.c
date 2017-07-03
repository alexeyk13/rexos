#include "dhcps.h"
#include "tcpips_private.h"
#include "sys_config.h"
#include "object.h"
#include "endian.h"
#include "udp.h"
#include "arp.h"

#include "ip.h"
#include "mac.h"

#define DHCP_OPTION_DNSSERVER       (1<< 0)
#define DHCP_OPTION_SERVERID        (1<< 1)
#define DHCP_OPTION_ROUTER          (1<< 2)
#define DHCP_OPTION_SUBNETMASK      (1<< 3)
#define DHCP_OPTION_LEASETIME       (1<< 4)

#define DHCP_OPTION_DHCP_NAK  0
#define DHCP_OPTION_DHCP_OFFER  ( DHCP_OPTION_DNSSERVER |   \
                                   DHCP_OPTION_SERVERID |   \
                                   DHCP_OPTION_ROUTER |     \
                                   DHCP_OPTION_SUBNETMASK | \
                                   DHCP_OPTION_LEASETIME)

#define DHCP_OPTION_DHCP_INFORM  ( DHCP_OPTION_DNSSERVER |   \
                                   DHCP_OPTION_SERVERID |   \
                                   DHCP_OPTION_ROUTER |     \
                                   DHCP_OPTION_SUBNETMASK)

//------- pool procedures -----
static void dhcp_init_pool(DHCP_POOL* pool, IP* start_ip)
{
    uint32_t i;
    for (i = DHCP_POOL_SIZE; i > 0; pool++, i--)
    {
        pool->mac.u32.hi = pool->mac.u32.lo = 0;
        pool->ip.u32.ip = start_ip->u32.ip + ((i-1) << 24);
    }
}

static uint32_t dhcp_get_ip(DHCP_POOL* pool, MAC* mac, bool is_assign)
{
    uint32_t i;
    uint32_t prom_ip = 0;
    DHCP_POOL* void_entry;
    if ((mac->u32.hi == 0) && (mac->u32.lo == 0))
        return 0;
    for (i = DHCP_POOL_SIZE; i > 0; pool++, i--)
    {
        if ((mac->u32.hi == pool->mac.u32.hi) && (mac->u32.lo == pool->mac.u32.lo))
            return pool->ip.u32.ip;
        if ((pool->mac.u32.hi == 0) && (pool->mac.u32.lo == 0))
        {
            prom_ip = pool->ip.u32.ip;
            void_entry = pool;
        }

    }
    if (prom_ip && is_assign)
    {
        void_entry->mac.u32.hi = mac->u32.hi;
        void_entry->mac.u32.lo = mac->u32.lo;
    }
    return prom_ip;
}

static void dhcp_release_ip(DHCP_POOL* pool, MAC* mac)
{
    uint32_t i;
    for (i = 0; i < DHCP_POOL_SIZE; pool++, i++)
    {
        if ((mac->u32.hi == pool->mac.u32.hi) && (mac->u32.lo == pool->mac.u32.lo))
            pool->mac.u32.hi = pool->mac.u32.lo = 0;
    }
}
//------- options -----
static void* dhcp_add_option_u32(uint8_t* ptr, uint32_t value, uint8_t option)
{
    uint32_t* ptr32;
    *ptr++ = option;
    *ptr++ = 4;
    ptr32 = (uint32_t*)ptr;
    *ptr32++ = value;
    return ptr32;
}

static uint32_t dhcp_get_option_u32(uint8_t* ptr, uint8_t option)
{
    uint32_t size = 0;
    while (1)
    {
        uint8_t opt = *ptr++;
        uint8_t len = *ptr++;
        size += len;
        if (opt == DHCP_END)
            return 0;
        if (size > DHCP_OPTION_LEN)
            return 0;
        if (opt == option)
        {
            if (len < 4)
                return 0;
            return *(uint32_t*)ptr;
        }
        ptr += len;
    }
}

static void dhcp_add_options(TCPIPS* tcpips, uint8_t* ptr, uint32_t option_msk)
{
    if (option_msk & DHCP_OPTION_DNSSERVER)
        ptr = dhcp_add_option_u32(ptr, tcpips->ips.ip.u32.ip, DHCP_DNSSERVER);
    if (option_msk & DHCP_OPTION_SERVERID)
        ptr = dhcp_add_option_u32(ptr, tcpips->ips.ip.u32.ip, DHCP_SERVERID);
    if (option_msk & DHCP_OPTION_ROUTER)
        ptr = dhcp_add_option_u32(ptr, tcpips->ips.ip.u32.ip, DHCP_ROUTER);
    if (option_msk & DHCP_OPTION_SUBNETMASK)
        ptr = dhcp_add_option_u32(ptr, tcpips->dhcps.net_mask.u32.ip, DHCP_SUBNETMASK);
    if (option_msk & DHCP_OPTION_LEASETIME)
        ptr = dhcp_add_option_u32(ptr, HTONL(DHCP_LEASE_TIME), DHCP_LEASETIME);
    *ptr = DHCP_END;
}
//----------------- DHCP request ------------
bool dhcps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    DHCP_TYPE* hdr = io_data(io);
    uint8_t* ptr;
    uint8_t msg_type;
    if (io->data_size < sizeof(DHCP_TYPE))
        return false;
    if (hdr->magic != DHCP_MAGIC)
        return false;
    if (hdr->op != DHCP_BOOT_REQUEST)
        return false;
    if ((hdr->msg_code != DHCP_MESSAGETYPE) || (hdr->msg_len != 1))
        return false;
    hdr->op = DHCP_BOOT_REPLY;
    hdr->secs = 0;
    ptr = hdr->options;
    msg_type = hdr->msg_type;
    hdr->msg_type = DHCP_OFFER;
    hdr->yiaddr.u32.ip = dhcp_get_option_u32(ptr, DHCP_IPADDRESS);
    memset(hdr->options, 0, DHCP_OPTION_LEN + 1);
    io->data_size = sizeof(DHCP_TYPE) + DHCP_OPTION_LEN;
#if (DHCPS_DEBUG)
    printf("DHCPS: request type %d from ", msg_type);
    mac_print((MAC*)&hdr->chaddr);
    printf(" yiaddr ");
    ip_print((const IP*)&hdr->yiaddr.u32.ip);
    printf("\n");
#endif
    switch (msg_type)
    {
    case DHCP_REQUEST: // send ACK or NAK
        if ((hdr->yiaddr.u32.ip == 0) || (hdr->yiaddr.u32.ip != dhcp_get_ip(tcpips->dhcps.pool, (MAC*)&hdr->chaddr, false)))
        {
            hdr->msg_type = DHCP_NAK;
            hdr->yiaddr.u32.ip = 0;
            hdr->ciaddr.u32.ip = 0;
            dhcp_add_options(tcpips, ptr, DHCP_OPTION_DHCP_NAK);
#if (DHCPS_DEBUG)
    printf("DHCPS: NAK \n");
#endif
        } else
        {
            hdr->msg_type = DHCP_ACK;
            dhcp_add_options(tcpips, ptr, DHCP_OPTION_DHCP_OFFER);
#if (DHCPS_DEBUG)
    printf("DHCPS: ACK \n");
#endif
        }
        break;
    case DHCP_DISCOVER: // send OFFER
        hdr->yiaddr.u32.ip = dhcp_get_ip(tcpips->dhcps.pool, (MAC*)&hdr->chaddr, true);
        if (hdr->yiaddr.u32.ip == 0)
        {
#if (DHCPS_DEBUG)
            printf("DHCPS: pool full! /n");
#endif
            return false;
        }
        dhcp_add_options(tcpips, ptr, DHCP_OPTION_DHCP_OFFER);
#if (DHCPS_DEBUG)
        printf("DHCPS: OFFER IP ");
        ip_print((const IP*)&hdr->yiaddr.u32.ip);
        printf("\n");
#endif
        break;
    case DHCP_INFORM:
        if ((hdr->ciaddr.u32.ip == 0) || (hdr->ciaddr.u32.ip != dhcp_get_ip(tcpips->dhcps.pool, (MAC*)&hdr->chaddr, false)))
            return false;
        hdr->msg_type = DHCP_ACK;
        hdr->yiaddr.u32.ip = 0;
        dhcp_add_options(tcpips, ptr, DHCP_OPTION_DHCP_INFORM);
        break;
    case DHCP_DECLINE:
        case DHCP_RELEASE:
        dhcp_release_ip(tcpips->dhcps.pool, (MAC*)&hdr->chaddr);
        return false;
    default:
        return false;
    }
    return true;
}

void dhcps_init(TCPIPS* tcpips)
{
    memset(tcpips->dhcps.pool, 0, sizeof(tcpips->dhcps.pool));
}

void dhcps_request(TCPIPS* tcpips, IPC* ipc)
{
    if (HAL_ITEM(ipc->cmd) == IPC_WRITE)
    {
        dhcp_init_pool(tcpips->dhcps.pool, (IP*)&ipc->param2);
        tcpips->dhcps.net_mask.u32.ip = ipc->param3;
    }
}

