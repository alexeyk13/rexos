/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IP_H
#define IP_H

#include <stdint.h>
#include "ipc.h"

#pragma pack(push, 1)

typedef union {
    uint8_t u8[4];
    struct {
        uint32_t ip;
    }u32;
} IP;

#pragma pack(pop)

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

#define IP_MAKE(a, b, c, d)                             (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define LOCALHOST                                       (IP_MAKE(127, 0, 0, 1))
#define BROADCAST                                       (IP_MAKE(255, 255, 255, 255))

typedef enum {
    IP_SET = IPC_USER,
    IP_GET,
    //notification of ip interface up/down
    IP_UP,
    IP_DOWN,
    IP_ENABLE_FIREWALL,
    IP_DISABLE_FIREWALL
}IP_IPCS;

void ip_print(const IP* ip);
uint16_t ip_checksum(void *buf, unsigned int size);
bool ip_compare(const IP* ip1, const IP* ip2, const IP* mask);
void ip_set(HANDLE tcpip, const IP* ip);
void ip_get(HANDLE tcpip, IP* ip);
void ip_enable_firewall(HANDLE tcpip, const IP* src, const IP* mask);
void ip_disable_firewall(HANDLE tcpip);

#endif // IP_H
