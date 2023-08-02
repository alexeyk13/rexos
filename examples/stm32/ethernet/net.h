/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
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
