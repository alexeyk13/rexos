/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg.h"
#include "kipc.h"
#include "kprocess.h"
#include "../userspace/error.h"
#include <string.h>

#define IPC_ITEM(process, num)              ((IPC*)((unsigned int)(process) + sizeof(PROCESS) + (num) * sizeof(IPC)))

int kipc_index(PROCESS* process, HANDLE wait_process)
{
    int i;
    for (i = process->kipc.rb.tail; i != process->kipc.rb.head; i = RB_ROUND_BACK(&process->kipc.rb, i - 1))
        if (IPC_ITEM(process, i)->process == wait_process || wait_process == 0)
            return i;
    return -1;
}

void kipc_post(IPC* ipc)
{
    PROCESS* receiver = (PROCESS*)ipc->process;
    PROCESS* sender = kprocess_get_current();
    IPC* cur;
    CHECK_MAGIC(receiver, MAGIC_PROCESS);
    if (receiver->kipc.rb.size > 0)
    {
        if (!rb_is_full(&receiver->kipc.rb))
        {
            cur = IPC_ITEM(receiver, rb_put(&receiver->kipc.rb));
            cur->cmd = ipc->cmd;
            cur->param1 = ipc->param1;
            cur->param2 = ipc->param2;
            cur->process = (HANDLE)sender;
            if (kipc_index(receiver, cur->process) >= 0)
                kprocess_wakeup(receiver);
        }
        else
        {
            //on overflow set error on both: receiver and sender
            kprocess_error(sender, ERROR_IPC_OVERFLOW);
            kprocess_error(receiver, ERROR_IPC_OVERFLOW);
            //TODO DBG
        }

    }
    else
        kprocess_error(sender, ERROR_NOT_SUPPORTED);
}

void kipc_peek(IPC* ipc, HANDLE wait_process)
{
    IPC tmp;
    int i;
    PROCESS* process = kprocess_get_current();
    if (process->kipc.rb.size > 0)
    {
        i = kipc_index(process, wait_process);
        if (i >= 0)
        {
            for(; i != process->kipc.rb.tail; i= RB_ROUND(&process->kipc.rb, i + 1))
            {
                //swap
                memcpy(&tmp, IPC_ITEM(process, i), sizeof(IPC));
                memcpy(IPC_ITEM(process, i), IPC_ITEM(process, RB_ROUND(&process->kipc.rb, i + 1)), sizeof(IPC));
                memcpy(IPC_ITEM(process, RB_ROUND(&process->kipc.rb, i + 1)), &tmp, sizeof(IPC));
            }
            memcpy(ipc, IPC_ITEM(process, rb_get(&process->kipc.rb)), sizeof(IPC));
        }
        else
            error(ERROR_IPC_NOT_FOUND);
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void kipc_wait(TIME* time, HANDLE wait_process)
{
    PROCESS* process = kprocess_get_current();
    if (process->kipc.rb.size > 0)
    {
        process->kipc.wait_process = wait_process;
        if (kipc_index(process, wait_process) < 0)
            kprocess_sleep(time, PROCESS_SYNC_IPC, process);
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void kipc_post_wait(IPC* ipc, TIME* time)
{
    kipc_post(ipc);
    if (kprocess_get_current()->heap->error == ERROR_OK)
        kipc_wait(time, ipc->process);
}
