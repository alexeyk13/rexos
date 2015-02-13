/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_ETH_H
#define STM32_ETH_H

/*
    STM32 Ethernet driver. Warning! It's just scratch, not ready yet.
*/

#include "../../userspace/process.h"
#include "../../userspace/eth.h"
#include <stdint.h>

typedef struct {
    bool active;
    ETH_CONN_TYPE conn;
    HANDLE arp, timer;
    HANDLE rx_block[2], tx_block[2];
    uint8_t rx_block_count, tx_block_count;
    uint8_t mac[MAC_SIZE];
} ETH_DRV;

extern const REX __STM32_ETH;


#endif // STM32_ETH_H
