/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_ETH_H
#define STM32_ETH_H

#include "../../userspace/process.h"

typedef struct {
    unsigned int stub;
} ETH_DRV;

extern const REX __STM32_ETH;


#endif // STM32_ETH_H
