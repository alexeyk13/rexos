/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MAGIC_H
#define MAGIC_H

/*
        magic.h - magics for kernel marks
*/

#define MAGIC_MEM_POOL_ENTRY                        0xbde4e2f1
#define MAGIC_MEM_POOL_ALIGN_SPACE                  0x851ab238
#define MAGIC_MEM_POOL_UNUSED                       0xed8bb5a9
#define MAGIC_RANGE_TOP                             0x27f0d1c2
#define MAGIC_RANGE_BOTTOM                          0xb45f8dc3

#define MAGIC_TIMER                                 0xbecafcf5
#define MAGIC_THREAD                                0x7de32076
#define MAGIC_MUTEX                                 0xd0cc6e26
#define MAGIC_EVENT                                 0x57e198c7
#define MAGIC_SEM                                   0xabfd92d9
#define MAGIC_QUEUE                                 0x6b54bbeb

#define MAGIC_NODE                                  0x8dea2a1e
#define MAGIC_MOUNT_POINT                           0xacb46e04

#define MAGIC_UNINITIALIZED                         0xcdcdcdcd
#define MAGIC_UNINITIALIZED_BYTE                    0xcd

#endif // MAGIC_H
