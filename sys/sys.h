/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_H
#define SYS_H

#include "../userspace/ipc.h"
#include "../userspace/lib/types.h"

typedef enum {
    SYS_SET_STDIO = IPC_SYSTEM + 1,                             //!< Will be called for objects, created before global stdout STREAM is set
    SYS_GET_OBJECT,                                             //!< Get system object. Temporaily solution before FS is ready
    SYS_SET_OBJECT,                                             //!< Set system object. Temporaily solution before FS is ready
    SYS_GET_INFO
}SYS_IPCS;

typedef enum {
    SYS_OBJECT_POWER,
    SYS_OBJECT_GPIO,
    SYS_OBJECT_TIMER,
    SYS_OBJECT_UART,
    SYS_OBJECT_STDOUT_STREAM,
    SYS_OBJECT_STDIN_STREAM
}SYS_OBJECT;

#endif // SYS_H
