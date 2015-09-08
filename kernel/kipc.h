/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KIPC_H
#define KIPC_H

#include "../userspace/ipc.h"

typedef struct _KPROCESS KPROCESS;

typedef struct {
    //process, we are waiting for. Can be INVALID_HANDLE, then waiting from any process
    KPROCESS* wait_process;
    unsigned int cmd, param1;
}KIPC;

//called from kernel directly
void kipc_init(KPROCESS* kprocess);
void kipc_post_process(IPC* ipc, KPROCESS* sender);

//called from svc
void kipc_post(IPC* ipc);
void kipc_wait(KPROCESS* wait_process, unsigned int cmd, unsigned int param1);
void kipc_call(IPC* ipc);

void kipc_lock_release(KPROCESS* kprocess);

#endif // KIPC_H
