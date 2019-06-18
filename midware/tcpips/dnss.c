/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.
*/

#include "dnss.h"
#include "tcpips_private.h"
#include "sys_config.h"
#include "object.h"
#include "endian.h"
#include "udps.h"

static const char server_name[] = {3, 'R', 'e', 'x', 2, 'O', 'S', 0};

void dnss_init(TCPIPS* tcpips)
{
    memset(tcpips->dnss.name, 0, 64);
}

static int dns_to_strcmp(uint8_t* dns_ptr, char* name)
{
    int len = 0;
    int chunk = *dns_ptr++;
    while (len < DNS_MAX_NAME_LEN && chunk)
    {
        *name++ = *dns_ptr++;
        len++;
        if(--chunk == 0)
        {
            *name++ = '.';
            len++;
            chunk = *dns_ptr++;
        }
    }
    if(len)
    {
        len++;
        name--;
    }
    *name = 0;
    return len;

}

static bool dns_cmp(uint8_t* dns_ptr, const char* name)
{
    uint8_t len;
    bool next = false;
    while (1)
    {
        len = *dns_ptr++;
        if (len == 0)
        {
            if (*name == 0)
                return true;
            else
                return false;
        }
        if (next)
        {
            if (*name++ != '.')
                return false;
        }
        next = true;

        do
        {
            if (*dns_ptr++ != *name++)
                return false;
            len--;
        } while (len);
    }
}
static bool dns_ptr_cmp(uint8_t* dns_ptr, IP* ip)
{
    uint8_t b;
    uint8_t num = 0;
    uint32_t res = 0;
    uint32_t cnt = 0;
    while (1)
    {
        b = *dns_ptr++;
        if (b < 10)
        {
            res = num + (res << 8);
            cnt++;
            num = 0;
            continue;
        }
        if ((b >= '0') && (b <= '9'))
        {
            num = num * 10 + b - '0';
        } else
            break;
    }
    if ((cnt == 5) && (res == ip->u32.ip))
        return true;
    else
        return false;
}

// true - need replay
bool dnss_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    DNS_HEADER* hdr;
    DNS_ANSWER* ans;
    void * ptr;
    uint16_t type;

    char name[DNS_MAX_NAME_LEN];

    if (io->data_size < (sizeof(DNS_HEADER) + 5))
        return false;
    ptr = io_data(io);
    hdr = ptr;
    if (hdr->qdcount != HTONS(1))
        return false;
    if (hdr->ancount != HTONS(0))
        return false;
    hdr->flags |= HTONS(DNS_FLAG_RESPONSE|DNS_FLAG_REQ_AVALIBLE);
    hdr->ancount = HTONS(1);
    hdr->nscount = HTONS(0);
    hdr->arcount = HTONS(0);
    ptr = hdr->name;
    ptr += dns_to_strcmp(ptr, name);
    type = *(uint16_t*)ptr;
    ptr+=4;
    ans = ptr;
    ans->compress = HTONS(0xC00C);
    ans->class = HTONS(DNS_CLASS_IN);
#if (DNSS_DEBUG)
    printf("DNSS: request type %d name:%s\n", HTONS(type), name);
#endif

    if (type == HTONS(DNS_TYPE_PTR))
    {
        if (!dns_ptr_cmp(hdr->name, &tcpips->ips.ip)) // if wrong IP - no answer
            return false;
        ans->type = HTONS(DNS_TYPE_PTR);
        ans->ttl = HTONL(320);
        ans->len = HTONS(sizeof(server_name));
        memcpy(&ans->ip, server_name, sizeof(server_name));

        io->data_size = (ptr - io_data(io)) + sizeof(DNS_ANSWER) + sizeof(server_name) - sizeof(IP);
        return true;
    }

    if ((type == HTONS(DNS_TYPE_A)) && (dns_cmp(hdr->name, tcpips->dnss.name) || dns_cmp(hdr->name, tcpips->dnss.name_www)))
    {
        ans->type = HTONS(DNS_CLASS_IN);
        ans->ttl = HTONL(32);
        ans->len = HTONS(sizeof(IP));
        ans->ip = tcpips->ips.ip.u32.ip;
        io->data_size = (ptr - io_data(io)) + sizeof(DNS_ANSWER);
#if (DNSS_DEBUG)
    printf("DNSS: match!\n");
#endif

    } else
    {
        hdr->ancount = HTONS(0);
        hdr->flags |= HTONS(DNS_ERROR_REFUSED);
        io->data_size = (ptr - io_data(io));
    }
    return true;
}

void dnss_request(TCPIPS* tcpips, IPC* ipc)
{
    if (HAL_ITEM(ipc->cmd) == IPC_WRITE)
    {
        char* name = (char*)io_data((IO*)ipc->param2);
        if (strlen(name) > 63)
            name[63] = 0;
        strcpy(tcpips->dnss.name, name);
        sprintf(tcpips->dnss.name_www, "www.%s",name);
    }
}
