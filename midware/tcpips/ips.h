/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IPS_H
#define IPS_H

#include "tcpips.h"
#include "../../userspace/ip.h"
#include "../../userspace/ipc.h"
#include "../../userspace/array.h"
#include "sys_config.h"

#define IP_FRAME_MAX_DATA_SIZE                          (TCPIP_MTU - sizeof(IP_HEADER))

//Internet Control Message       [RFC792,JBP]
#define PROTO_ICMP                                      1
//Internet Group Management     [RFC1112,JBP]
#define PROTO_IGMP                                      2
//Gateway-to-Gateway              [RFC823,MB]
#define PROTO_GGP                                       3
//IP in IP (encasulation)               [JBP]
#define PROTO_IP                                        4
//Stream                 [RFC1190,IEN119,JWF]
#define PROTO_ST                                        5
//Transmission Control           [RFC793,JBP]
#define PROTO_TCP                                       6
//UCL                                    [PK]
#define PROTO_UCL                                       7
//Exterior Gateway Protocol     [RFC888,DLM1]
#define PROTO_EGP                                       8
//any private interior gateway          [JBP]
#define PROTO_IGP                                       9
//BBN RCC Monitoring                    [SGC]
#define PROTO_BBN_RCC_MON                               10
//Network Voice Protocol         [RFC741,SC3]
#define PROTO_NVP_II                                    11
//PUP                             [PUP,XEROX]
#define PROTO_PUP                                       12
//ARGUS                                [RWS4]
#define PROTO_ARGUS                                     13
//EMCON                                 [BN7]
#define PROTO_EMCON                                     14
//Cross Net Debugger            [IEN158,JFH2]
#define PROTO_XNET                                      15
//Chaos                                 [NC3]
#define PROTO_CHAOS                                     16
//User Datagram                  [RFC768,JBP]
#define PROTO_UDP                                       17
//Multiplexing                    [IEN90,JBP]
#define PROTO_MUX                                       18
//DCN Measurement Subsystems           [DLM1]
#define PROTO_DCN_MEAS                                  19
//Host Monitoring                [RFC869,RH6]
#define PROTO_HMP                                       20
//Packet Radio Measurement              [ZSU]
#define PROTO_PRM                                       21
//XEROX NS IDP               [ETHERNET,XEROX]
#define PROTO_XNS_IDP                                   22
//Trunk-1                              [BWB6]
#define PROTO_TRUNK_1                                   23
//Trunk-2                              [BWB6]
#define PROTO_TRUNK_2                                   24
//Leaf-1                               [BWB6]
#define PROTO_LEAF_1                                    25
//Leaf-2                               [BWB6]
#define PROTO_LEAF_2                                    26
//Reliable Data Protocol         [RFC908,RH6]
#define PROTO_RDP                                       27
//Internet Reliable Transaction  [RFC938,TXM]
#define PROTO_IRTP                                      28
//ISO Transport Protocol Class 4 [RFC905,RC77]
#define PROTO_ISO_TP4                                   29
//Bulk Data Transfer Protocol    [RFC969,DDC1]
#define PROTO_NETBLT                                    30
//MFE Network Services Protocol  [MFENET,BCH2]
#define PROTO_MFE_NSP                                   31
//MERIT Internodal Protocol             [HWB]
#define PROTO_MERIT_INP                                 32
//Sequential Exchange Protocol        [JC120]
#define PROTO_SEP                                       33
//Third Party Connect Protocol         [SAF3]
#define PROTO_3PC                                       34
//Inter-Domain Policy Routing Protocol [MXS1]
#define PROTO_IDPR                                      35
//XTP                                   [GXC]
#define PROTO_XTP                                       36
//Datagram Delivery Protocol            [WXC]
#define PROTO_DDP                                       37
//IDPR Control Message Transport Proto [MXS1]
#define PROTO_IDPR_CMTP                                 38
//TP++ Transport Protocol               [DXF]
#define PROTO_TPPP                                      39
//IL Transport Protocol                [DXP2]
#define PROTO_IL                                        40
//Simple Internet Protocol              [SXD]
#define PROTO_SIP                                       41
//Source Demand Routing Protocol       [DXE1]
#define PROTO_SDRP                                      42
//SIP Source Route                      [SXD]
#define PROTO_SIP_SR                                    43
//SIP Fragment                          [SXD]
#define PROTO_SIP_FRAG                                  44
//Inter-Domain Routing Protocol   [Sue Hares]
#define PROTO_IDRP                                      45
//Reservation Protocol           [Bob Braden]
#define PROTO_RSVP                                      46
//General Routing Encapsulation     [Tony Li]
#define PROTO_GRE                                       47
//Mobile Host Routing Protocol[David Johnson]
#define PROTO_MHRP                                      48
//BNA                          [Gary Salamon]
#define PROTO_BNA                                       49
//SIPP Encap Security Payload [Steve Deering]
#define PROTO_SIPP_ESP                                  50
//SIPP Authentication Header  [Steve Deering]
#define PROTO_SIPP_AH                                   51
//Integrated Net Layer Security  TUBA [GLENN]
#define PROTO_I_NLSP                                    52
//IP with Encryption                    [JI6]
#define PROTO_SWIPE                                     53
//NBMA Next Hop Resolution Protocol
#define PROTO_NHRP                                      54
//any host internal protocol            [JBP]
#define PROTO_INTERNAL                                  61
//CFTP                            [CFTP,HCF2]
#define PROTO_CFTP                                      62
//any local network                     [JBP]
#define PROTO_LOCAL                                     63
//SATNET and Backroom EXPAK             [SHB]
#define PROTO_SAT_EXPAK                                 64
//Kryptolan                            [PXL1]
#define PROTO_KRYPTOLAN                                 65
//MIT Remote Virtual Disk Protocol      [MBG]
#define PROTO_RVD                                       66
//Internet Pluribus Packet Core         [SHB]
#define PROTO_IPPC                                      67
//any distributed file system           [JBP]
#define PROTO_DFS                                       68
//SATNET Monitoring                     [SHB]
#define PROTO_SAT_MON                                   69
//VISA Protocol                        [GXT1]
#define PROTO_VISA                                      70
//Internet Packet Core Utility          [SHB]
#define PROTO_IPCV                                      71
//Computer Protocol Network Executive  [DXM2]
#define PROTO_CPNX                                      72
//Computer Protocol Heart Beat         [DXM2]
#define PROTO_CPHB                                      73
//Wang Span Network                     [VXD]
#define PROTO_WSN                                       74
//Packet Video Protocol                 [SC3]
#define PROTO_PVP                                       75
//Backroom SATNET Monitoring            [SHB]
#define PROTO_BR_SAT_MON                                76
//SUN ND PROTOCOL-Temporary             [WM3]
#define PROTO_SUN_ND                                    77
//WIDEBAND Monitoring                   [SHB]
#define PROTO_WB_MON                                    78
//WIDEBAND EXPAK                        [SHB]
#define PROTO_WB_EXPAK                                  79
//ISO Internet Protocol                 [MTR]
#define PROTO_ISO_IP                                    80
//VMTP                                 [DRC3]
#define PROTO_VMTP                                      81
//SECURE-VMTP                          [DRC3]
#define PROTO_SECURE_VMTP                               82
//VINES                                 [BXH]
#define PROTO_VINES                                     83
//TTP                                   [JXS]
#define PROTO_TTP                                       84
//NSFNET-IGP                            [HWB]
#define PROTO_NSFNET_IGP                                85
//Dissimilar Gateway Protocol     [DGP,ML109]
#define PROTO_DGP                                       86
//TCF                                  [GAL5]
#define PROTO_TCF                                       87
//IGRP                            [CISCO,GXS]
#define PROTO_IGRP                                      88
//OSPFIGP                      [RFC1583,JTM4]
#define PROTO_OSPFIGP                                   89
//Sprite RPC Protocol            [SPRITE,BXW]
#define PROTO_SPRITE_RPC                                90
//Locus Address Resolution Protocol     [BXH]
#define PROTO_LARP                                      91
//Multicast Transport Protocol          [SXA]
#define PROTO_MTP                                       92
//AX.25 Frames                         [BK29]
#define PROTO_AX_25                                     93
//IP-within-IP Encapsulation Protocol   [JI6]
#define PROTO_IPIP                                      94
//Mobile Internetworking Control Pro.   [JI6]
#define PROTO_MICP                                      95
//Semaphore Communications Sec. Pro.    [HXH]
#define PROTO_SCC_SP                                    96
//Ethernet-within-IP Encapsulation     [RXH1]
#define PROTO_ETHERIP                                   97
//Encapsulation Header         [RFC1241,RXB3]
#define PROTO_ENCAP                                     98
//any private encryption scheme         [JBP]
#define PROTO_PRIVATE                                   99
//GMTP                                 [RXB5]
#define PROTO_GMTP                                      100

typedef struct {
    uint16_t id;
    bool up;
#if (IP_FRAGMENTATION)
    unsigned int io_allocated;
    ARRAY* free_io;
    ARRAY* assembly_io;
#endif //IP_FRAGMENTATION
} IPS;

#pragma pack(push, 1)

typedef struct {
    //Version and IHL
    uint8_t ver_ihl;
    //Type of Service
    uint8_t tos;
    //Total length
    uint8_t total_len_be[2];
    //Identification
    uint8_t id_be[2];
    //Flags, fragment offset
    uint8_t flags_offset_be[2];
    uint8_t ttl;
    uint8_t proto;
    uint8_t header_crc_be[2];
    IP src;
    IP dst;
    //options may follow
} IP_HEADER;

#pragma pack(pop)

typedef struct {
    uint16_t hdr_size;
    uint16_t proto;
#if (IP_FRAGMENTATION)
    bool is_long;
#endif //IP_FRAGMENTATION
} IP_STACK;

//from tcpip
void ips_init(TCPIPS* tcpips);
void ips_request(TCPIPS* tcpips, IPC* ipc);
void ips_link_changed(TCPIPS* tcpips, bool link);
#if (IP_FRAGMENTATION)
void ips_timer(TCPIPS* tcpips, unsigned int seconds);
#endif //IP_FRAGMENTATION

//allocate IP io. If more than (MTU - MAC header - IP header) and fragmentation enabled, will be allocated long frame
IO* ips_allocate_io(TCPIPS* tcpips, unsigned int size, uint8_t proto);
//release previously allocated io. IO is not actually freed, just put in queue of free ios
void ips_release_io(TCPIPS* tcpips, IO* io);
void ips_tx(TCPIPS* tcpips, IO* io, const IP* dst);

//from mac
void ips_rx(TCPIPS* tcpips, IO* io);

#endif // IPS_H
