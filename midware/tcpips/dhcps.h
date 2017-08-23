/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DHCPS_H
#define DHCPS_H

#include "config.h"
#include "tcpips.h"
#include "mac.h"
#include "stdlib.h"
#include "stdio.h"
#include "endian.h"
#include <string.h>

#define DHCP_POOL_SIZE   5

#define DHCP_SERVER_PORT      67
#define DHCP_CLIENT_PORT      68

#define DHCP_BOOT_REQUEST     1
#define DHCP_BOOT_REPLY       2

// DHCP message type 
#define DHCP_DISCOVER         1
#define DHCP_OFFER            2
#define DHCP_REQUEST          3
#define DHCP_DECLINE          4
#define DHCP_ACK              5
#define DHCP_NAK              6
#define DHCP_RELEASE          7
#define DHCP_INFORM           8

#define DHCP_MAGIC            0x63538263
#define DHCP_LEASE_TIME       86000
#define DHCP_OPTION_LEN       160

enum DHCP_OPTIONS {
    DHCP_PAD = 0,
    DHCP_SUBNETMASK = 1,
    DHCP_ROUTER = 3,
    DHCP_DNSSERVER = 6,
    DHCP_HOSTNAME = 12,
    DHCP_DNSDOMAIN = 15,
    DHCP_MTU = 26,
    DHCP_BROADCAST = 28,
    DHCP_PERFORMROUTERDISC = 31,
    DHCP_STATICROUTE = 33,
    DHCP_NISDOMAIN = 40,
    DHCP_NISSERVER = 41,
    DHCP_NTPSERVER = 42,
    DHCP_VENDOR = 43,
    DHCP_IPADDRESS = 50,
    DHCP_LEASETIME = 51,
    DHCP_OPTIONSOVERLOADED = 52,
    DHCP_MESSAGETYPE = 53,
    DHCP_SERVERID = 54,
    DHCP_PARAMETERREQUESTLIST = 55,
    DHCP_MESSAGE = 56,
    DHCP_MAXMESSAGESIZE = 57,
    DHCP_RENEWALTIME = 58,
    DHCP_REBINDTIME = 59,
    DHCP_CLASSID = 60,
    DHCP_CLIENTID = 61,
    DHCP_USERCLASS = 77,  // RFC 3004 
    DHCP_FQDN = 81,
    DHCP_DNSSEARCH = 119, // RFC 3397 
    DHCP_CSR = 121,       // RFC 3442 
    DHCP_MSCSR = 249,     // MS code for RFC 3442
    DHCP_END = 255,
};

typedef struct {
    uint8_t op;          // packet opcode type 
    uint8_t htype;       // hardware addr type
    uint8_t hlen;        // hardware addr length 
    uint8_t hops;        // gateway hops
    uint32_t xid;        // transaction ID
    uint16_t secs;       // seconds since boot began
    uint16_t flags;
    IP ciaddr;           // client IP address
    IP yiaddr;           // 'your' IP address 
    IP siaddr;           // server IP address
    IP giaddr;           // gateway IP address
    uint8_t chaddr[16];  // client hardware address
    uint8_t legacy[192];
    uint32_t magic;
    uint8_t msg_code;
    uint8_t msg_len;
    uint8_t msg_type;
    uint8_t options[0];  // options area outside header
} DHCP_TYPE;

typedef struct {
    MAC mac;
    IP ip;
} DHCP_POOL;

typedef struct {
    DHCP_POOL pool[DHCP_POOL_SIZE];
    IP net_mask;
} _DHCPS;

void dhcps_init(TCPIPS* tcpips);
bool dhcps_rx(TCPIPS* tcpips, IO* io, IP* src);
void dhcps_request(TCPIPS* tcpips, IPC* ipc);

#endif //DHCPS_H
