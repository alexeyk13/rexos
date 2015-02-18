/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "arp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/block.h"
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

static const MAC __MAC_BROADCAST =                  {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
static const MAC __MAC_REQUEST =                    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void arp_init(TCPIP* tcpip)
{
    tcpip->arp.stub = 0;
}

void arp_test(TCPIP* tcpip)
{
    printf("ARP test\n\r");
    uint8_t* buf;
    const MAC* own_mac;
    unsigned int mac_size;
    HANDLE block = tcpip_allocate_block(tcpip);
    uint8_t* raw = block_open(block);
    if (raw == NULL)
        return;
    mac_size = mac_prepare_tx(tcpip, raw, &__MAC_BROADCAST, ETHERTYPE_ARP);
    buf = raw + mac_size;
    //hrd
    buf[0] = (ARP_HRD_ETHERNET >> 8) & 0xff;
    buf[1] = ARP_HRD_ETHERNET & 0xff;
    //pro
    buf[2] = (ETHERTYPE_IP >> 8) & 0xff;
    buf[3] = ETHERTYPE_IP & 0xff;
    //hln
    buf[4] = MAC_SIZE;
    //pln
    buf[5] = IP_SIZE;
    //op
    buf[6] = (ARP_REQUEST >> 8) & 0xff;
    buf[7] = ARP_REQUEST & 0xff;
    //sha
    own_mac = tcpip_mac(tcpip);
    ARP_SHA(buf)->u32.hi = own_mac->u32.hi;
    ARP_SHA(buf)->u32.lo = own_mac->u32.lo;
    //spa
    ARP_SPA(buf)->u32.ip = IP_MAKE(192, 168, 1, 199);
    //tha
    ARP_THA(buf)->u32.hi = __MAC_REQUEST.u32.hi;
    ARP_THA(buf)->u32.lo = __MAC_REQUEST.u32.lo;
    //tpa
    ARP_TPA(buf)->u32.ip = IP_MAKE(192, 168, 1, 1);


    dump(raw, ARP_HEADER_SIZE + MAC_SIZE * 2 + IP_SIZE * 2 + mac_size);
    tcpip_tx(tcpip, block, ARP_HEADER_SIZE + MAC_SIZE * 2 + IP_SIZE * 2 + mac_size);

    printf("ARP test ok\n\r");
}

void arp_rx(TCPIP* tcpip, uint8_t* buf, unsigned int size, HANDLE block)
{
    if (size < ARP_HEADER_SIZE)
    {
        tcpip_release_block(tcpip, block);
        return;
    }
    if (ARP_PRO(buf) != ETHERTYPE_IP || ARP_HLN(buf) != MAC_SIZE || ARP_PLN(buf) != IP_SIZE)
    {
        tcpip_release_block(tcpip, block);
        return;
    }
    switch (ARP_OP(buf))
    {
    case ARP_REQUEST:
        printf("ARP request: ");
        print_mac(ARP_SHA(buf));
        printf("(");
        print_ip(ARP_SPA(buf));
        printf(") -> ");
        print_mac(ARP_THA(buf));
        printf("(");
        print_ip(ARP_TPA(buf));
        printf(")\n\r");

        if (tcpip->arp.stub == 0)
        {
            tcpip->arp.stub = 1;
//            arp_test(tcpip);
        }

        break;
    case ARP_REPLY:
        //TODO: filter if not own mac and api

        printf("ARP REPLY: ");
        print_mac(ARP_SHA(buf));
        printf("(");
        print_ip(ARP_SPA(buf));
        printf(") -> ");
        print_mac(ARP_THA(buf));
        printf("(");
        print_ip(ARP_TPA(buf));
        printf(")\n\r");
        break;
    }
    tcpip_release_block(tcpip, block);
}

const MAC* arp_resolve(TCPIP* tcpip, const IP* ip)
{
    return NULL;
}
