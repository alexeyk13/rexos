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

typedef struct {
    bool active;
    HANDLE rx_block[2], tx_block[2];
    uint8_t rx_block_count, tx_block_count;
} ETH_DRV;

extern const REX __STM32_ETH;


#endif // STM32_ETH_H
