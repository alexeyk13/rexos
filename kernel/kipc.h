/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KIPC_H
#define KIPC_H

#include "../userspace/ipc.h"
#include "../userspace/lib/rb.h"
#include "../userspace/lib/time.h"

typedef struct {
    RB rb;
    //process, we are waiting for. Can be INVALID_HANDLE, then waiting from any process
    HANDLE wait_process;
    IPC* ipc;
}KIPC;

//called from svc
void kipc_post(IPC* ipc);
void kipc_read(IPC* ipc, TIME* time, HANDLE wait_process);
void kipc_call(IPC* ipc, TIME* time);

#endif // KIPC_H
