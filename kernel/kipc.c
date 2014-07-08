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
#include "kernel.h"


#define IPC_ITEM(process, num)              ((IPC*)((unsigned int)(process) + sizeof(PROCESS) + (num) * sizeof(IPC)))

int kipc_index(PROCESS* process, HANDLE wait_process)
{
    int i;
    for (i = process->kipc.rb.tail; i != process->kipc.rb.head; i = RB_ROUND_BACK(&process->kipc.rb, i - 1))
        if (IPC_ITEM(process, i)->process == wait_process || wait_process == 0)
            return i;
    return -1;
}

void kipc_wait_process(PROCESS* process, TIME* time, HANDLE wait_process)
{
#if (KERNEL_IPC_DEBUG)
    if (wait_process == (HANDLE)process)
        printk("Warning: calling wait IPC with receiver same as caller can cause deadlock! process: %s\n\r", PROCESS_NAME(process->heap));
#endif
    if (process->kipc.rb.size > 0)
    {
        if (kipc_index(process, wait_process) < 0)
        {
            process->kipc.wait_process = wait_process;
            kprocess_sleep(process, time, PROCESS_SYNC_IPC, process);
        }
    }
    else
        kprocess_error(process, ERROR_NOT_SUPPORTED);
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
            cur->param3 = ipc->param3;
            cur->process = (HANDLE)sender;
            if (receiver->kipc.wait_process == (HANDLE)sender || receiver->kipc.wait_process == 0)
            {
                receiver->kipc.wait_process = (unsigned int)-1;
                kprocess_wakeup(receiver);
            }
        }
        else
        {
            //on overflow set error on both: receiver and sender
            kprocess_error(sender, ERROR_IPC_OVERFLOW);
            kprocess_error(receiver, ERROR_IPC_OVERFLOW);
#if (KERNEL_IPC_DEBUG)
            printk("Error: receiver %s IPC overflow!\n\r", PROCESS_NAME(receiver->heap));
#endif
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
            kprocess_error(process, ERROR_IPC_NOT_FOUND);
    }
    else
        kprocess_error(process, ERROR_NOT_SUPPORTED);
}

void kipc_wait(TIME* time, HANDLE wait_process)
{
    kipc_wait_process(kprocess_get_current(), time, wait_process);
}

void kipc_post_wait(IPC* ipc, TIME* time)
{
    PROCESS* process = kprocess_get_current();
    HANDLE wait_process = (HANDLE)ipc->process;
    kipc_post(ipc);
    kipc_wait_process(process, time, wait_process);
}
