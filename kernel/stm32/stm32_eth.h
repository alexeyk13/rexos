/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_ETH_H
#define STM32_ETH_H

/*
    STM32 Ethernet driver.
*/

#include "../../userspace/eth.h"
#include "../../userspace/io.h"
#include <stdint.h>
#include "sys_config.h"
#include "stm32_exo.h"

#pragma pack(push, 1)

typedef struct {
    uint32_t ctl;
    uint32_t size;
    void* buf1;
    void* buf2_ndes;
} ETH_DESCRIPTORS;

#pragma pack(pop)

#define ETH_TDES_OWN                    (1 << 31)
#define ETH_TDES_IC                     (1 << 30)
#define ETH_TDES_LS                     (1 << 29)
#define ETH_TDES_FS                     (1 << 28)
#define ETH_TDES_DC                     (1 << 27)
#define ETH_TDES_DP                     (1 << 26)
#define ETH_TDES_TTSE                   (1 << 25)

#define ETH_TDES_CIC_DISABLE            (0 << 22)
#define ETH_TDES_CIC_IP                 (1 << 22)
#define ETH_TDES_CIC_IP_PAYLOAD         (2 << 22)
#define ETH_TDES_CIC_ALL                (3 << 22)

#define ETH_TDES_TER                    (1 << 21)
#define ETH_TDES_TCH                    (1 << 20)

#define ETH_TDES_TTSS                   (1 << 17)
#define ETH_TDES_IHE                    (1 << 16)
#define ETH_TDES_ES                     (1 << 15)
#define ETH_TDES_JT                     (1 << 14)
#define ETH_TDES_FF                     (1 << 13)
#define ETH_TDES_IPE                    (1 << 12)
#define ETH_TDES_LCA                    (1 << 11)
#define ETH_TDES_NC                     (1 << 10)
#define ETH_TDES_LCO                    (1 << 9)
#define ETH_TDES_EC                     (1 << 8)
#define ETH_TDES_VF                     (1 << 7)

#define ETH_TDES_CC_POS                 3
#define ETH_TDES_CC_MASK                (0xf << 3)

#define ETH_TDES_ED                     (1 << 2)
#define ETH_TDES_UF                     (1 << 1)
#define ETH_TDES_DB                     (1 << 0)

#define ETH_TDES_TBS1_POS               0
#define ETH_TDES_TBS1_MASK              (0xfff << 0)

#define ETH_TDES_TBS2_POS               16
#define ETH_TDES_TBS2_MASK              (0xfff << 16)


#define ETH_RDES_OWN                    (1 << 31)
#define ETH_RDES_AFM                    (1 << 30)

#define ETH_RDES_FL_POS                 16
#define ETH_RDES_FL_MASK                (0x3fff << 16)

#define ETH_RDES_ES                     (1 << 15)
#define ETH_RDES_DE                     (1 << 14)
#define ETH_RDES_SAF                    (1 << 13)
#define ETH_RDES_LE                     (1 << 12)
#define ETH_RDES_OE                     (1 << 11)
#define ETH_RDES_VLAN                   (1 << 10)
#define ETH_RDES_FS                     (1 << 9)
#define ETH_RDES_LS                     (1 << 8)
#define ETH_RDES_IPHCE                  (1 << 7)
#define ETH_RDES_LCO                    (1 << 6)
#define ETH_RDES_FT                     (1 << 5)
#define ETH_RDES_RWT                    (1 << 4)
#define ETH_RDES_RE                     (1 << 3)
#define ETH_RDES_DBE                    (1 << 2)
#define ETH_RDES_CE                     (1 << 1)
#define ETH_RDES_PCE                    (1 << 0)

#define ETH_RDES_DIC                    (1 << 31)

#define ETH_RDES_RBS1_POS               0
#define ETH_RDES_RBS1_MASK              (0xfff << 0)

#define ETH_RDES_RBS2_POS               16
#define ETH_RDES_RBS2_MASK              (0xfff << 16)

#define ETH_RDES_RER                    (1 << 15)
#define ETH_RDES_RCH                    (1 << 14)

typedef struct {
#if (ETH_DOUBLE_BUFFERING)
    IO* tx[2];
    IO* rx[2];
    ETH_DESCRIPTORS tx_des[2], rx_des[2];
#else
    ETH_DESCRIPTORS tx_des, rx_des;
    IO* tx;
    IO* rx;
#endif
    ETH_CONN_TYPE conn;
    unsigned int cc;
    HANDLE timer;
    bool timer_pending;
    HANDLE tcpip;
    bool connected;
    MAC mac;
    uint8_t phy_addr;
#if (ETH_DOUBLE_BUFFERING)
    uint8_t cur_rx, cur_tx;
#endif
} ETH_DRV;

void stm32_eth_init(EXO* exo);
void stm32_eth_request(EXO* exo, IPC* ipc);

#endif // STM32_ETH_H
