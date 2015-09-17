/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IP_H
#define IP_H

#include <stdint.h>
#include "ipc.h"

#define IP_SIZE                                     4

#pragma pack(push, 1)

typedef union {
    uint8_t u8[IP_SIZE];
    struct {
        uint32_t ip;
    }u32;
} IP;

#pragma pack(pop)

#define IP_MAKE(a, b, c, d)                         (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24))

typedef enum {
    IP_SET = IPC_USER,
    IP_GET,

    IP_HAL_MAX
}IP_IPCS;

void ip_set(HANDLE tcpip, const IP* ip);
void ip_get(HANDLE tcpip, IP* ip);

#endif // IP_H
