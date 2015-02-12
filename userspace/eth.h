/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ETH_H
#define ETH_H

#include <stdint.h>

typedef enum {
    ETH_10_HALF = 0,
    ETH_10_FULL,
    ETH_100_HALF,
    ETH_100_FULL,
    ETH_AUTO,
    ETH_LOOPBACK,
    //for status only
    ETH_NO_LINK,
    ETH_REMOTE_FAULT
} ETH_CONN_TYPE;

typedef struct {
    ETH_CONN_TYPE conn;
    uint8_t mac[6];
} ETH_OPEN_STRUCT;

#endif // ETH_H
