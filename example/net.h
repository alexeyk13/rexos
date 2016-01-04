/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef NET_H
#define NET_H

#include "app.h"
#include "../../rexos/userspace/types.h"
#include "../../rexos/userspace/ipc.h"
#include "../../rexos/userspace/io.h"
#include "../../rexos/userspace/ip.h"

typedef struct {
    HANDLE tcpip, listener, connection;
    IO* io;
    IP remote_addr;
} NET;

void net_init(APP* app);
void net_request(APP* app, IPC* ipc);

#endif // NET_H
