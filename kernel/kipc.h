/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KIPC_H
#define KIPC_H

#include "../userspace/ipc.h"
#include "kprocess.h"

//called from kprocess
void kipc_init(KPROCESS* process);
void kipc_lock_release(KPROCESS* process);

void kipc_post(HANDLE sender, IPC* ipc);
void kipc_post_exo(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);
void kipc_wait(HANDLE process, HANDLE wait_process, unsigned int cmd, unsigned int param1);
void kipc_call(HANDLE process, IPC* ipc);

#endif // KIPC_H
