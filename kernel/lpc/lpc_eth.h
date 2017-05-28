/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_ETH_H
#define LPC_ETH_H

/*
    LPC18xx Ethernet driver.
*/

#include "../../userspace/process.h"
#include "../../userspace/eth.h"
#include "../../userspace/io.h"
#include <stdint.h>
#include "sys_config.h"
#include "lpc_exo.h"
#include <stdbool.h>

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
    unsigned int processing;
    bool timeout;
} ETH_DRV;

void lpc_eth_init(EXO* exo);
void lpc_eth_request(EXO* exo, IPC* ipc);


#endif // LPC_ETH_H
