/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KIPC_H
#define KIPC_H

#include "../userspace/ipc.h"
#include "../userspace/rb.h"
#include "../userspace/time.h"

typedef struct _PROCESS KPROCESS;

typedef struct {
    RB rb;
    //process, we are waiting for. Can be INVALID_HANDLE, then waiting from any process
    HANDLE wait_process;
    unsigned int cmd;
    IPC* ipc;
}KIPC;

//called from kernel directly
void kipc_init(HANDLE handle);
void kipc_post_process(IPC* ipc, HANDLE sender);
void kipc_read_process(KPROCESS* kprocess, IPC* ipc, SYSTIME* time, HANDLE wait_process, unsigned int cmd);

//called from svc
void kipc_post(IPC* ipc);
void kipc_read(IPC* ipc, SYSTIME* time, HANDLE wait_process);
void kipc_call(IPC* ipc, SYSTIME* time);

void kipc_lock_release(HANDLE kprocess);

#endif // KIPC_H
