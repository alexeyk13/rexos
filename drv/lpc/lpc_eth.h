/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_ETH_H
#define LPC_ETH_H

/*
    STM32 Ethernet driver.
*/

#include "../../userspace/process.h"
#include "../../userspace/eth.h"
#include "../../userspace/io.h"
#include <stdint.h>
#include "sys_config.h"

#pragma pack(push, 1)

typedef struct {
    uint32_t ctl;
    uint32_t size;
    void* buf1;
    void* buf2_ndes;
    uint32_t ext_status;
    uint32_t reserved;
    uint32_t time_stamp_lo;
    uint32_t time_stamp_hi;
} ETH_DESCRIPTOR;

#pragma pack(pop)

#define ETH_TDES0_OWN                               (1 << 31)
#define ETH_TDES0_IC                                (1 << 30)
#define ETH_TDES0_LS                                (1 << 29)
#define ETH_TDES0_FS                                (1 << 28)
#define ETH_TDES0_DC                                (1 << 27)
#define ETH_TDES0_DP                                (1 << 26)
#define ETH_TDES0_TTSE                              (1 << 25)
#define ETH_TDES0_TER                               (1 << 21)
#define ETH_TDES0_TCH                               (1 << 20)
#define ETH_TDES0_TTSS                              (1 << 17)
#define ETH_TDES0_IHE                               (1 << 16)
#define ETH_TDES0_ES                                (1 << 15)
#define ETH_TDES0_JT                                (1 << 14)
#define ETH_TDES0_FF                                (1 << 13)
#define ETH_TDES0_IPE                               (1 << 12)
#define ETH_TDES0_LCA                               (1 << 11)
#define ETH_TDES0_NC                                (1 << 10)
#define ETH_TDES0_LCO                               (1 << 9)
#define ETH_TDES0_EC                                (1 << 8)
#define ETH_TDES0_VF                                (1 << 7)

#define ETH_TDES0_CC_POS                            3
#define ETH_TDES0_CC_MASK                           (0xf << 3)

#define ETH_TDES0_ED                                (1 << 2)
#define ETH_TDES0_UF                                (1 << 1)
#define ETH_TDES0_DB                                (1 << 0)

#define ETH_TDES1_TBS1_POS                          0
#define ETH_TDES1_TBS1_MASK                         (0x1fff << 0)

#define ETH_TDES1_TBS2_POS                          16
#define ETH_TDES1_TBS2_MASK                         (0x1fff << 16)


#define ETH_RDES0_OWN                               (1 << 31)
#define ETH_RDES0_AFM                               (1 << 30)

#define ETH_RDES0_FL_POS                            16
#define ETH_RDES0_FL_MASK                           (0x3fff << 16)

#define ETH_RDES0_ES                                (1 << 15)
#define ETH_RDES0_DE                                (1 << 14)
#define ETH_RDES0_SAF                               (1 << 13)
#define ETH_RDES0_LE                                (1 << 12)
#define ETH_RDES0_OE                                (1 << 11)
#define ETH_RDES0_VLAN                              (1 << 10)
#define ETH_RDES0_FS                                (1 << 9)
#define ETH_RDES0_LS                                (1 << 8)
#define ETH_RDES0_TSA                               (1 << 7)
#define ETH_RDES0_LCO                               (1 << 6)
#define ETH_RDES0_FT                                (1 << 5)
#define ETH_RDES0_RWT                               (1 << 4)
#define ETH_RDES0_RE                                (1 << 3)
#define ETH_RDES0_DBE                               (1 << 2)
#define ETH_RDES0_CE                                (1 << 1)
#define ETH_RDES0_ESA                               (1 << 0)

#define ETH_RDES1_DIC                               (1 << 31)

#define ETH_RDES1_RBS1_POS                          0
#define ETH_RDES1_RBS1_MASK                         (0x1ffc << 0)

#define ETH_RDES1_RBS2_POS                          16
#define ETH_RDES1_RBS2_MASK                         (0x1ffc << 16)

#define ETH_RDES1_RER                               (1 << 15)
#define ETH_RDES1_RCH                               (1 << 14)

#define ETH_RDES4_PTPVERSION                        (1 << 13)
#define ETH_RDES4_PTPTYPE                           (1 << 12)

#define ETH_RDES4_MTP_POS                           8
#define ETH_RDES4_MTP_MASK                          (0xf << 8)

#define ETH_RDES4_MTP_NO_PTP                        (0x0 << 8)
#define ETH_RDES4_MTP_SYNC                          (0x1 << 8)
#define ETH_RDES4_MTP_FOLLOW_UP                     (0x2 << 8)
#define ETH_RDES4_MTP_DELAY_REQ                     (0x3 << 8)
#define ETH_RDES4_MTP_DELAY_RESP                    (0x4 << 8)
#define ETH_RDES4_MTP_PDELAY_REQ                    (0x5 << 8)
#define ETH_RDES4_MTP_PDELAY_RESP                   (0x6 << 8)
#define ETH_RDES4_MTP_PDELAY_RESP_FOLLOW_UP         (0x7 << 8)
#define ETH_RDES4_MTP_ANNOUNCE                      (0x8 << 8)
#define ETH_RDES4_MTP_MANAGEMENT                    (0x9 << 8)
#define ETH_RDES4_MTP_SIGNALLING                    (0x9 << 8)

#define ETH_RDES4_IPV6                              (1 << 7)
#define ETH_RDES4_IPV4                              (1 << 6)

typedef struct {
#if (ETH_DOUBLE_BUFFERING)
    IO* tx[2];
    IO* rx[2];
    ETH_DESCRIPTOR tx_des[2], rx_des[2];
#else
    ETH_DESCRIPTOR tx_des, rx_des;
    IO* tx;
    IO* rx;
#endif
    ETH_CONN_TYPE conn;
    HANDLE tcpip, timer;
    bool connected;
    MAC mac;
    uint8_t phy_addr;
#if (ETH_DOUBLE_BUFFERING)
    uint8_t cur_rx, cur_tx;
#endif
} ETH_DRV;

#endif // LPC_ETH_H
