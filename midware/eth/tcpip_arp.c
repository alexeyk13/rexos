/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpip_arp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/block.h"
#include "tcpip_mac.h"
#include "tcpip_ip.h"

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

void tcpip_arp_init(TCPIP* tcpip)
{
    tcpip->arp.stub = 0;
}

void arp_test(TCPIP* tcpip)
{
    printf("ARP test\n\r");
    const MAC* own_mac;
    TCPIP_IO io;
    if (tcpip_mac_allocate_io(tcpip, &io) == NULL)
        return;
    //hrd
    io.buf[0] = (ARP_HRD_ETHERNET >> 8) & 0xff;
    io.buf[1] = ARP_HRD_ETHERNET & 0xff;
    //pro
    io.buf[2] = (ETHERTYPE_IP >> 8) & 0xff;
    io.buf[3] = ETHERTYPE_IP & 0xff;
    //hln
    io.buf[4] = MAC_SIZE;
    //pln
    io.buf[5] = IP_SIZE;
    //op
    io.buf[6] = (ARP_REQUEST >> 8) & 0xff;
    io.buf[7] = ARP_REQUEST & 0xff;
    //sha
    own_mac = tcpip_mac(tcpip);
    ARP_SHA(io.buf)->u32.hi = own_mac->u32.hi;
    ARP_SHA(io.buf)->u32.lo = own_mac->u32.lo;
    //spa
    ARP_SPA(io.buf)->u32.ip = tcpip_ip(tcpip)->u32.ip;
    //tha
    ARP_THA(io.buf)->u32.hi = __MAC_REQUEST.u32.hi;
    ARP_THA(io.buf)->u32.lo = __MAC_REQUEST.u32.lo;
    //tpa
    ARP_TPA(io.buf)->u32.ip = IP_MAKE(192, 168, 1, 1);

    io.size = ARP_HEADER_SIZE + MAC_SIZE * 2 + IP_SIZE * 2;
    dump(io.buf, io.size);
    tcpip_mac_tx(tcpip, &io, &__MAC_BROADCAST, ETHERTYPE_ARP);

    printf("ARP test ok\n\r");
}

//TODO: arp_info(TCPIP* tcpip);
void tcpip_arp_rx(TCPIP* tcpip, TCPIP_IO* io)
{
    if (io->size < ARP_HEADER_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    if (ARP_PRO(io->buf) != ETHERTYPE_IP || ARP_HLN(io->buf) != MAC_SIZE || ARP_PLN(io->buf) != IP_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    switch (ARP_OP(io->buf))
    {
    case ARP_REQUEST:
        printf("ARP request: ");
        print_mac(ARP_SHA(io->buf));
        printf("(");
        print_ip(ARP_SPA(io->buf));
        printf(") -> ");
        print_mac(ARP_THA(io->buf));
        printf("(");
        print_ip(ARP_TPA(io->buf));
        printf(")\n\r");

        if (tcpip->arp.stub == 0)
        {
            tcpip->arp.stub = 1;
            arp_test(tcpip);
            process_info();
        }

        break;
    case ARP_REPLY:
        //TODO: filter if not own mac and api

        printf("ARP REPLY: ");
        print_mac(ARP_SHA(io->buf));
        printf("(");
        print_ip(ARP_SPA(io->buf));
        printf(") -> ");
        print_mac(ARP_THA(io->buf));
        printf("(");
        print_ip(ARP_TPA(io->buf));
        printf(")\n\r");
        break;
    }
    tcpip_release_io(tcpip, io);
}

const MAC* tcpip_arp_resolve(TCPIP* tcpip, const IP* ip)
{
    return NULL;
}
