/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ips.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "../../userspace/error.h"
#include "icmps.h"
#include <string.h>
#include "udps.h"
#include "tcps.h"

#if (IP_FRAGMENTATION)

typedef struct _ASSEMBLY_SLOT {
    struct _ASSEMBLY_SLOT* next;
    unsigned int size;
} ASSEMBLY_SLOT;

#define LONG_IP_FRAME_MAX_DATA_SIZE             (IP_MAX_LONG_SIZE - sizeof(IP_HEADER))
#define LONG_IP_FRAME_MAX_SIZE                  (IP_MAX_LONG_SIZE + sizeof(MAC_HEADER) + sizeof(IP_STACK) + sizeof(ASSEMBLY_SLOT))

typedef struct {
    IO* io;
    ASSEMBLY_SLOT* head;
    unsigned int ttl;
    IP src;
    uint16_t id;
    bool got_last;
} IPS_ASSEMBLY;
#endif //IP_FRAGMENTATION

#define IP_DF                                   (1 << 6)
#define IP_MF                                   (1 << 5)
#define IP_FLAGS_MASK                           (7 << 5)

void ips_init(TCPIPS* tcpips)
{
    tcpips->ips.ip.u32.ip = IP_MAKE(0, 0, 0, 0);
    tcpips->ips.up = false;

#if (IP_FIREWALL)
    tcpips->ips.firewall_enabled = false;
#endif //IP_FIREWALL

#if (IP_FRAGMENTATION)
    tcpips->ips.io_allocated = 0;
    array_create(&tcpips->ips.free_io, sizeof(IO*), 1);
    array_create(&tcpips->ips.assembly_io, sizeof(IPS_ASSEMBLY), 1);
#endif //IP_FRAGMENTATION
}

#if (IP_FRAGMENTATION)
static IO* ips_allocate_long(TCPIPS* tcpips)
{
    IO* io = NULL;
    if (array_size(tcpips->ips.free_io))
    {
        io = *((IO**)array_at(tcpips->ips.free_io, array_size(tcpips->ips.free_io) - 1));
        array_remove(&tcpips->ips.free_io, array_size(tcpips->ips.free_io) - 1);
    }
    else if (tcpips->ips.io_allocated < IP_MAX_LONG_PACKETS)
    {
        io = io_create(LONG_IP_FRAME_MAX_SIZE);
        if (io != NULL)
            ++tcpips->ips.io_allocated;
    }
    else
        error(ERROR_TOO_MANY_HANDLES);
    return io;
}

static void ips_release_long(TCPIPS* tcpips, IO* io)
{
    IO** iop;
    io_reset(io);
    iop = array_append(&tcpips->ips.free_io);
    if (iop)
        *iop = io;
}

static inline IPS_ASSEMBLY* ips_allocate_assembly(TCPIPS* tcpips, const IP* src, uint16_t id)
{
    IO* io;
    IPS_ASSEMBLY* as;
    ASSEMBLY_SLOT* asl;
    if (array_size(tcpips->ips.assembly_io) >= IP_MAX_LONG_PACKETS)
    {
        error(ERROR_TOO_MANY_HANDLES);
        return NULL;
    }
    io = ips_allocate_long(tcpips);
    if (io == NULL)
        return NULL;
    as = array_append(&tcpips->ips.assembly_io);
    as->got_last = false;
    as->io = io;
    as->head = io_data(io);
    asl = io_data(io);
    asl->next = NULL;
    asl->size = LONG_IP_FRAME_MAX_DATA_SIZE;
    as->ttl = tcpips->seconds + IP_FRAGMENTATION_ASSEMBLY_TIMEOUT;
    as->src.u32.ip = src->u32.ip;
    as->id = id;
    as->got_last = false;
    return as;
}

static inline void ips_free_assembly(TCPIPS* tcpips, const IP* src, uint16_t id)
{
    int i;
    IPS_ASSEMBLY* as;
    for (i = 0; i < array_size(tcpips->ips.assembly_io); ++i)
    {
        as = array_at(tcpips->ips.assembly_io, i);
        if (as->src.u32.ip == src->u32.ip && as->id == id)
        {
            array_remove(&tcpips->ips.assembly_io, i);
            return;
        }
    }
}

static IPS_ASSEMBLY* ips_find_assembly(TCPIPS* tcpips, const IP* src, uint16_t id)
{
    int i;
    IPS_ASSEMBLY* as;
    for (i = 0; i < array_size(tcpips->ips.assembly_io); ++i)
    {
        as = array_at(tcpips->ips.assembly_io, i);
        if (as->src.u32.ip == src->u32.ip && as->id == id)
            return as;
    }
    //not found? allocate
    return ips_allocate_assembly(tcpips, src, id);
}

static bool ips_insert_assembly(IPS_ASSEMBLY* as, IO* io, unsigned int offset)
{
    ASSEMBLY_SLOT* asl;
    ASSEMBLY_SLOT* prev;
    ASSEMBLY_SLOT* n;
    unsigned int cur_offset;
    for (asl = as->head, prev = NULL; asl != NULL; prev = asl, asl = asl->next)
    {
        cur_offset = (unsigned int)asl - (unsigned int)io_data(as->io);
        if (cur_offset > offset)
            break;
        //fit
        if (cur_offset <= offset && cur_offset - offset + asl->size >= io->data_size)
        {
            //insert in top of free slot
            if (cur_offset == offset)
            {
                n = (ASSEMBLY_SLOT*)((unsigned int)asl + io->data_size);
                if (prev)
                    prev->next = n;
                else
                    as->head = n;
                n->next = asl->next;
                n->size = asl->size - io->data_size;
            }
            //no need to change prev
            else
            {
                //bottom
                if (offset - cur_offset + io->data_size == asl->size)
                    asl->size -= io->data_size;
                //in the middle
                else
                {
                    n = (ASSEMBLY_SLOT*)((unsigned int)asl + io->data_size + offset - cur_offset);
                    n->next = asl->next;
                    n->size = asl->size - (offset - cur_offset) - io->data_size;
                    asl->size = offset - cur_offset;
                }
            }
            memcpy((void*)((unsigned int)asl + io->data_size + offset - cur_offset), io_data(io), io->data_size);
            return true;
        }
    }
    return false;
}
#endif //IP_FRAGMENTATION

static inline void ips_set(TCPIPS* tcpips, uint32_t ip)
{
    tcpips->ips.ip.u32.ip = ip;
}

#if (IP_FIREWALL)
void ips_enable_firewall(TCPIPS* tcpips, IP* src, IP* mask)
{
    if (tcpips->ips.firewall_enabled)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tcpips->ips.src.u32.ip = src->u32.ip;
    tcpips->ips.mask.u32.ip = mask->u32.ip;
    tcpips->ips.firewall_enabled = true;
}

void ips_disable_firewall(TCPIPS* tcpips)
{
    if (!tcpips->ips.firewall_enabled)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    tcpips->ips.firewall_enabled = false;
}
#endif //IP_FIREWALL

void ips_request(TCPIPS* tcpips, IPC* ipc)
{
#if (IP_FIREWALL)
    IP src, mask;
#endif //IP_FIREWALL
    switch (HAL_ITEM(ipc->cmd))
    {
    case IP_SET:
        ips_set(tcpips, ipc->param2);
        break;
    case IP_GET:
        ipc->param2 = tcpips->ips.ip.u32.ip;
        break;
#if (IP_FIREWALL)
    case IP_ENABLE_FIREWALL:
        src.u32.ip = ipc->param2;
        mask.u32.ip = ipc->param3;
        ips_enable_firewall(tcpips, &src, &mask);
        break;
    case IP_DISABLE_FIREWALL:
        ips_disable_firewall(tcpips);
        break;
#endif //IP_FIREWALL
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void ips_link_changed(TCPIPS* tcpips, bool link)
{
    if (link)
    {
        tcpips->ips.up = true;
        tcpips->ips.id = 0;
        ipc_post_inline(tcpips->app, HAL_CMD(HAL_IP, IP_UP), 0, 0, 0);
    }
    else
    {
        tcpips->ips.up = false;
        ipc_post_inline(tcpips->app, HAL_CMD(HAL_IP, IP_DOWN), 0, 0, 0);
    }
}

#if (IP_FRAGMENTATION)
void ips_timer(TCPIPS* tcpips, unsigned int seconds)
{
    int i;
    IPS_ASSEMBLY* as;
    for (i = 0; i < array_size(tcpips->ips.assembly_io); ++i)
    {
        as = array_at(tcpips->ips.assembly_io, i);
        if (as->ttl < seconds)
        {
#if (IP_DEBUG)
            printf("IP: Fragment assembly timeout\n");
#endif //IP_DEBUG
            ips_release_long(tcpips, as->io);
            ips_free_assembly(tcpips, &as->src, as->id);
        }
    }
}
#endif //IP_FRAGMENTATION

IO* ips_allocate_io(TCPIPS* tcpips, unsigned int size, uint8_t proto)
{
    IP_STACK* ip_stack;
    IO* io = NULL;
    if (size <= IP_FRAME_MAX_DATA_SIZE)
    {
        io = macs_allocate_io(tcpips);
        if (!io)
            return NULL;
        ip_stack = io_push(io, sizeof(IP_STACK));

#if (IP_FRAGMENTATION)
        ip_stack->is_long = false;
#endif //IP_FRAGMENTATION
    }
#if (IP_FRAGMENTATION)
    else if (size <= LONG_IP_FRAME_MAX_DATA_SIZE)
    {
        io = ips_allocate_long(tcpips);
        if (!io)
            return NULL;
        ip_stack = io_push(io, sizeof(IP_STACK));
        ip_stack->is_long = true;
    }
#endif //IP_FRAGMENTATION
    else
    {
        error(ERROR_INVALID_PARAMS);
        return NULL;
    }

    //reserve space for IP header
    io->data_offset += sizeof(IP_HEADER);
    ip_stack->proto = proto;
    ip_stack->hdr_size = sizeof(IP_HEADER);
    return io;
}

void ips_release_io(TCPIPS* tcpips, IO* io)
{
#if (IP_FRAGMENTATION)
    IP_STACK* ip_stack = io_stack(io);
    if (ip_stack->is_long)
        ips_release_long(tcpips, io);
    else
#endif //IP_FRAGMENTATION
        tcpips_release_io(tcpips, io);
}

static void ips_tx_internal(TCPIPS* tcpips, IO* io, const IP* dst, unsigned int hdr_size)
{
    IP_HEADER* hdr;
    hdr = io_data(io);

    //VERSION, IHL
    hdr->ver_ihl = (0x4 << 4) | (hdr_size >> 2);
    //DSCP, ECN
    hdr->tos = 0;
    //total len
    short2be(hdr->total_len_be, io->data_size);
    //ttl
    hdr->ttl = 0xff;
    //src
    hdr->src.u32.ip = tcpips->ips.ip.u32.ip;
    //dst
    hdr->dst.u32.ip = dst->u32.ip;
    //update checksum
    short2be(hdr->header_crc_be, 0);
    short2be(hdr->header_crc_be, ip_checksum(io_data(io), hdr_size));

    routes_tx(tcpips, io, dst);
}

void ips_tx(TCPIPS* tcpips, IO* io, const IP* dst)
{
    IP_HEADER* hdr;
    IP_STACK* ip_stack;
#if (IP_FRAGMENTATION)
    unsigned int offset, cur;
    IO* fragment;
#endif //IP_FRAGMENTATION
    //drop if interface is not up
    if (!tcpips->ips.up)
    {
        ips_release_io(tcpips, io);
        return;
    }
    ip_stack = io_stack(io);
    io->data_offset -= ip_stack->hdr_size;
    io->data_size += ip_stack->hdr_size;
    hdr = io_data(io);

#if (IP_FRAGMENTATION)
    if (ip_stack->is_long)
    {
        for (offset = ip_stack->hdr_size; offset < io->data_size; offset += cur)
        {
            fragment = tcpips_allocate_io(tcpips);
            if (fragment == NULL)
            {
#if (IP_DEBUG)
                printf("IP: fragmentation failed - out of free blocks\n");
#endif //IP_DEBUG
                if (offset > ip_stack->hdr_size)
                    ++tcpips->ips.id;
                ips_release_io(tcpips, io);
                return;
            }
            cur = IP_FRAME_MAX_DATA_SIZE;
            if (offset + cur > io->data_size)
                cur = io->data_size - offset;
            //hdr
            memcpy(io_data(fragment), io_data(io), ip_stack->hdr_size);
            //data
            memcpy((uint8_t*)io_data(fragment) + ip_stack->hdr_size, (uint8_t*)io_data(io) + offset, cur);

            hdr = io_data(fragment);
            short2be(hdr->id_be, tcpips->ips.id);
            hdr->proto = ip_stack->proto;
            short2be(hdr->flags_offset_be, offset >> 3);
            //MF
            if (offset + cur < io->data_size)
                hdr->flags_offset_be[0] |= IP_FLAGS_MASK;
        }
        ips_release_io(tcpips, io);
        ++tcpips->ips.id;
        return;
    }
#endif //IP_FRAGMENTATION

    short2be(hdr->id_be, tcpips->ips.id++);
    hdr->proto = ip_stack->proto;
    //flags, offset
    hdr->flags_offset_be[0] = hdr->flags_offset_be[1] = 0;
    io_pop(io, sizeof(IP_STACK));
    ips_tx_internal(tcpips, io, dst, ip_stack->hdr_size);
}

static void ips_process(TCPIPS* tcpips, IO* io, IP* src)
{
    IP_STACK* ip_stack = io_stack(io);
#if (IP_DEBUG_FLOW)
    printf("IP: from ");
    ip_print(src);
    printf(", proto: %d, len: %d\n", ip_stack->proto, io->data_size);
#endif //IP_DEBUG_FLOW
    switch (ip_stack->proto)
    {
#if (ICMP)
    case PROTO_ICMP:
        icmps_rx(tcpips, io, src);
        break;
#endif //ICMP
#if (UDP)
    case PROTO_UDP:
        udps_rx(tcpips, io, src);
        break;
#endif //UDP
    case PROTO_TCP:
        tcps_rx(tcpips, io, src);
        break;
    default:
#if (IP_DEBUG)
        printf("IP: unhandled proto %d from ", ip_stack->proto);
        ip_print(src);
        printf("\n");
#endif //IP_DEBUG
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PROTOCOL, 0);
#endif //ICMP
        ips_release_io(tcpips, io);
    }
}

#if (IP_FRAGMENTATION)
static inline void ips_insert_fragment(TCPIPS* tcpips, IO* io, unsigned int offset, bool more)
{
    IP_HEADER* hdr;
    IPS_ASSEMBLY* as;
    IO* assembled;
    IP_STACK* ip_stack = io_stack(io);
    hdr = (IP_HEADER*)(((uint8_t*)io_data(io)) - ip_stack->hdr_size);
    as = ips_find_assembly(tcpips, &hdr->src, be2short(hdr->id_be));
    if (as == NULL)
    {
#if (IP_DEBUG)
        printf("IP: too many fragmented frames\n");
#endif //IP_DEBUG
        tcpips_release_io(tcpips, io);
        return;
    }
#if (IP_DEBUG_FLOW)
    printf("IP: fragmented frame insert: offset %d, more: %d\n", offset, more);
#endif //IP_DEBUG
    //fit?
    if (io->data_size + offset > LONG_IP_FRAME_MAX_DATA_SIZE)
    {
#if (IP_DEBUG)
        printf("IP: fragmented frame too big to fit\n");
#endif //IP_DEBUG
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PARAMETER, 2);
#endif //ICMP
        tcpips_release_io(tcpips, io);
        //assembly drop
        ips_release_long(tcpips, as->io);
        ips_free_assembly(tcpips, &as->src, as->id);
        return;
    }
    //first frame?
    if (offset == 0)
    {
        //unhide header
        io->data_offset -= ip_stack->hdr_size;
        io->data_size += ip_stack->hdr_size;
    }
    if (!ips_insert_assembly(as, io, offset))
    {
#if (IP_DEBUG)
        printf("IP: possible duplicated frame\n");
#endif //IP_DEBUG
        tcpips_release_io(tcpips, io);
        return;
    }
    tcpips_release_io(tcpips, io);
    if (!more)
        as->got_last = true;
    //got last and no fragments? received all
    if (as->got_last && as->head->next->next == NULL)
    {
#if (IP_DEBUG_FLOW)
        printf("IP: Assembly complete\n");
#endif //IP_DEBUG_FLOW
        assembled = as->io;
        assembled->data_size = (unsigned int)as->head - (unsigned int)io_data(as->io);
        ips_free_assembly(tcpips, &as->src, as->id);
        hdr = io_data(assembled);
        ip_stack = io_push(assembled, sizeof(IP_STACK));
        ip_stack->hdr_size = (hdr->ver_ihl & 0xf) << 2;
        ip_stack->proto = hdr->proto;
        ip_stack->is_long = true;
        //update header
        io->data_offset += ip_stack->hdr_size;
        io->data_size -= ip_stack->hdr_size;
        //total len
        short2be(hdr->total_len_be, io->data_size);
        //flags, offset
        hdr->flags_offset_be[0] = hdr->flags_offset_be[1] = 0;
        //update checksum
        short2be(hdr->header_crc_be, 0);
        short2be(hdr->header_crc_be, ip_checksum(io_data(assembled), ip_stack->hdr_size));
        ips_process(tcpips, assembled, &hdr->src);
    }
}
#endif //IP_FRAGMENTATION

void ips_rx(TCPIPS* tcpips, IO* io)
{
    IP src;
    uint16_t total_len, offset;
    uint8_t flags;
    IP_STACK* ip_stack;
    IP_HEADER* hdr = io_data(io);
    if (io->data_size < sizeof(IP_HEADER))
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    //some hardware sending packet more than MTU. Noise on line?
    if (io->data_size > TCPIP_MTU)
        io->data_size = TCPIP_MTU;
    ip_stack = io_push(io, sizeof(IP_STACK));

    ip_stack->hdr_size = (hdr->ver_ihl & 0xf) << 2;
#if (IP_FRAGMENTATION)
    ip_stack->is_long = false;
#endif //IP_FRAGMENTATION
#if (IP_CHECKSUM)
    //drop if checksum is invalid
    if (ip_checksum(io_data(io), ip_stack->hdr_size))
    {
        tcpips_release_io(tcpips, io);
        return;
    }
#endif

#if (IP_FIREWALL)
    if (tcpips->ips.firewall_enabled && !ip_compare(&hdr->src, &tcpips->ips.src, &tcpips->ips.mask))
    {
#if (IP_DEBUG)
        printf("IP: ");
        ip_print(&hdr->src);
        printf(" Firewall condition\n");
#endif //IP_DEBUG
        tcpips_release_io(tcpips, io);
        return;
    }
#endif //IP_FIREWALL
    src.u32.ip = hdr->src.u32.ip;
    total_len = be2short(hdr->total_len_be);
    ip_stack->proto = hdr->proto;

    //len more than MTU, inform host by ICMP and only than drop packet
    if (total_len > io->data_size)
    {
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PARAMETER, 2);
#endif //ICMP
        tcpips_release_io(tcpips, io);
        return;
    }

    io->data_size = total_len;
    io->data_offset += ip_stack->hdr_size;
    io->data_size -= ip_stack->hdr_size;
    if (((hdr->ver_ihl >> 4) != 4) || (ip_stack->hdr_size < sizeof(IP_HEADER)))
    {
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PARAMETER, 0);
#endif //ICMP
        tcpips_release_io(tcpips, io);
        return;
    }
#if(UDP_BROADCAST)
    if ((tcpips->ips.ip.u32.ip != hdr->dst.u32.ip) && ((hdr->dst.u32.ip !=BROADCAST) || (hdr->proto != PROTO_UDP )))
#else
    //unicast-only filter
    if (tcpips->ips.ip.u32.ip != hdr->dst.u32.ip)
#endif
    {
        tcpips_release_io(tcpips, io);
        return;
    }

    //ttl exceeded
    if (hdr->ttl == 0)
    {
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_TTL_EXCEED, 0);
#endif //ICMP
        tcpips_release_io(tcpips, io);
        return;
    }

    flags = hdr->flags_offset_be[0] & IP_FLAGS_MASK;
    offset = (((hdr->flags_offset_be[0] & ~IP_FLAGS_MASK) << 8) | hdr->flags_offset_be[1]) << 3;
    //drop all fragmented frames, inform by ICMP
    if ((flags & IP_MF) || offset)
    {
#if (IP_FRAGMENTATION)
        ips_insert_fragment(tcpips, io, offset, flags & IP_MF);
#else
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PARAMETER, 6);
#endif //ICMP
        tcpips_release_io(tcpips, io);
#endif //IP_FRAGMENTATION
        return;
    }

    ips_process(tcpips, io, &src);
}
