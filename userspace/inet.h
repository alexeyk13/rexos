/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef INET_H
#define INET_H

#define IP_SIZE                         4

#pragma pack(push, 1)

typedef union {
    uint8_t u8[IP_SIZE];
    struct {
        uint32_t ip;
    }u32;
} IP;

#pragma pack(pop)

#endif // INET_H
