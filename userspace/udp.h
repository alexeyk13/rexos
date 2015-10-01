/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UDP_H
#define UDP_H

#include "ip.h"

#pragma pack(push, 1)

typedef struct {
    IP remote_addr;
    uint16_t src_port, dst_port;
} UDP_STACK;

#pragma pack(pop)

#endif // UDP_H
