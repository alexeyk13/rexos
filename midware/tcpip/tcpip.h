/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_H
#define TCPIP_H

#include "../../userspace/process.h"
#include "../../userspace/types.h"

typedef struct _TCPIP TCPIP;

typedef struct {
    HANDLE block;
    uint8_t* buf;
    unsigned int size;
} TCPIP_IO;

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
//IGP         any private interior gateway          [JBP]
#define PROTO_IGP                                       9
//BBN-RCC-MON BBN RCC Monitoring                    [SGC]
#define PROTO_BBN_RCC_MON                               10
//NVP-II      Network Voice Protocol         [RFC741,SC3]
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
/*  31     MFE-NSP     MFE Network Services Protocol  [MFENET,BCH2]
  32     MERIT-INP   MERIT Internodal Protocol             [HWB]
  33     SEP         Sequential Exchange Protocol        [JC120]
  34     3PC         Third Party Connect Protocol         [SAF3]
  35     IDPR        Inter-Domain Policy Routing Protocol [MXS1]
  36     XTP         XTP                                   [GXC]
  37     DDP         Datagram Delivery Protocol            [WXC]
  38     IDPR-CMTP   IDPR Control Message Transport Proto [MXS1]
  39     TP++        TP++ Transport Protocol               [DXF]
  40     IL          IL Transport Protocol                [DXP2]
  41     SIP         Simple Internet Protocol              [SXD]
  42     SDRP        Source Demand Routing Protocol       [DXE1]
  43     SIP-SR      SIP Source Route                      [SXD]
  44     SIP-FRAG    SIP Fragment                          [SXD]
  45     IDRP        Inter-Domain Routing Protocol   [Sue Hares]
  46     RSVP        Reservation Protocol           [Bob Braden]
  47     GRE         General Routing Encapsulation     [Tony Li]
  48     MHRP        Mobile Host Routing Protocol[David Johnson]
  49     BNA         BNA                          [Gary Salamon]
  50     SIPP-ESP    SIPP Encap Security Payload [Steve Deering]
  51     SIPP-AH     SIPP Authentication Header  [Steve Deering]
  52     I-NLSP      Integrated Net Layer Security  TUBA [GLENN]
  53     SWIPE       IP with Encryption                    [JI6]
  54     NHRP        NBMA Next Hop Resolution Protocol
  61                 any host internal protocol            [JBP]
  62     CFTP        CFTP                            [CFTP,HCF2]
  63                 any local network                     [JBP]
  64     SAT-EXPAK   SATNET and Backroom EXPAK             [SHB]
  65     KRYPTOLAN   Kryptolan                            [PXL1]
  66     RVD         MIT Remote Virtual Disk Protocol      [MBG]
  67     IPPC        Internet Pluribus Packet Core         [SHB]
  68                 any distributed file system           [JBP]
  69     SAT-MON     SATNET Monitoring                     [SHB]
  70     VISA        VISA Protocol                        [GXT1]
  71     IPCV        Internet Packet Core Utility          [SHB]
  72     CPNX        Computer Protocol Network Executive  [DXM2]
  73     CPHB        Computer Protocol Heart Beat         [DXM2]
  74     WSN         Wang Span Network                     [VXD]
  75     PVP         Packet Video Protocol                 [SC3]
  76     BR-SAT-MON  Backroom SATNET Monitoring            [SHB]
  77     SUN-ND      SUN ND PROTOCOL-Temporary             [WM3]
  78     WB-MON      WIDEBAND Monitoring                   [SHB]
  79     WB-EXPAK    WIDEBAND EXPAK                        [SHB]
  80     ISO-IP      ISO Internet Protocol                 [MTR]
  81     VMTP        VMTP                                 [DRC3]
  82     SECURE-VMTP SECURE-VMTP                          [DRC3]
  83     VINES       VINES                                 [BXH]
  84     TTP         TTP                                   [JXS]
  85     NSFNET-IGP  NSFNET-IGP                            [HWB]
  86     DGP         Dissimilar Gateway Protocol     [DGP,ML109]
  87     TCF         TCF                                  [GAL5]
  88     IGRP        IGRP                            [CISCO,GXS]
  89     OSPFIGP     OSPFIGP                      [RFC1583,JTM4]
  90     Sprite-RPC  Sprite RPC Protocol            [SPRITE,BXW]
  91     LARP        Locus Address Resolution Protocol     [BXH]
  92     MTP         Multicast Transport Protocol          [SXA]
  93     AX.25       AX.25 Frames                         [BK29]
  94     IPIP        IP-within-IP Encapsulation Protocol   [JI6]
  95     MICP        Mobile Internetworking Control Pro.   [JI6]
  96     SCC-SP      Semaphore Communications Sec. Pro.    [HXH]
  97     ETHERIP     Ethernet-within-IP Encapsulation     [RXH1]
  98     ENCAP       Encapsulation Header         [RFC1241,RXB3]
  99                 any private encryption scheme         [JBP]
 100     GMTP        GMTP                                 [RXB5]
*/
extern const REX __TCPIP;

//allocate io. Find in queue of free blocks, or allocate new. Handle must be checked on return
uint8_t* tcpip_allocate_io(TCPIP* tcpip, TCPIP_IO* io);
//release previously allocated block. Block is not actually freed, just put in queue of free blocks
void tcpip_release_io(TCPIP* tcpip, TCPIP_IO* io);
//transmit. If tx operation is in place (2 tx for double buffering), io will be putted in queue for later processing
void tcpip_tx(TCPIP* tcpip, TCPIP_IO* io);

#endif // TCPIP_H
