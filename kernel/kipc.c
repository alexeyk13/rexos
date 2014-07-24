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

void kipc_read_process(PROCESS* process, IPC* ipc, TIME* time, HANDLE wait_process)
{
    int i;
    IPC tmp;
    if ((process != 0 && __KERNEL->context < 0) &&
        (((unsigned int)(time) < (unsigned int)(((PROCESS*)(process))->heap))
         || ((unsigned int)(time) + (sizeof(TIME)) >= (unsigned int)(((PROCESS*)(process))->heap) + ((PROCESS*)(process))->size))) \
    {
//        disable_interrupts();
//        printk("INVALID ADDRESS at %s, line %d\n\r", __FILE__, __LINE__);
        printk("INVALID ADDRESS at r, line %d\n\r", __LINE__);
//        printk("process: %s\n\r", PROCESS_NAME(process->heap));
        HALT();
    }
//    CHECK_ADDRESS(process, time, sizeof(TIME));
    CHECK_ADDRESS(process, ipc, sizeof(IPC));
#if (KERNEL_IPC_DEBUG)
    if (wait_process == (HANDLE)process)
        printk("Warning: calling wait IPC with receiver same as caller can cause deadlock! process: %s\n\r", PROCESS_NAME(process->heap));
#endif
    i = kipc_index(process, wait_process);
    //maybe already on queue? Peek.
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
    //no? sleep and wait
    else
    {
        process->kipc.ipc = ipc;
        process->kipc.wait_process = wait_process;
        kprocess_sleep(process, time, PROCESS_SYNC_IPC, process);
    }
}

void kipc_post_process(IPC* ipc, unsigned int sender)
{
    //NULL means kernel
    CHECK_ADDRESS(sender, ipc, sizeof(IPC));
    PROCESS* receiver = (PROCESS*)ipc->process;
    IPC* cur;
    CHECK_HANDLE(receiver, sizeof(PROCESS));
    CHECK_MAGIC(receiver, MAGIC_PROCESS);
    //already waiting?
    if (receiver->kipc.wait_process == sender || receiver->kipc.wait_process == 0)
    {
        receiver->kipc.wait_process = (unsigned int)-1;
        receiver->kipc.ipc->cmd = ipc->cmd;
        receiver->kipc.ipc->param1 = ipc->param1;
        receiver->kipc.ipc->param2 = ipc->param2;
        receiver->kipc.ipc->param3 = ipc->param3;
        receiver->kipc.ipc->process = sender;
        kprocess_wakeup(receiver);
    }
    else if (receiver->kipc.rb.size && !rb_is_full(&receiver->kipc.rb))
    {
        cur = IPC_ITEM(receiver, rb_put(&receiver->kipc.rb));
        cur->cmd = ipc->cmd;
        cur->param1 = ipc->param1;
        cur->param2 = ipc->param2;
        cur->param3 = ipc->param3;
        cur->process = sender;
    }
    else
    {
        kprocess_error(receiver, ERROR_OVERFLOW);
#if (KERNEL_IPC_DEBUG)
        printk("Error: receiver %s IPC overflow!\n\r", PROCESS_NAME(receiver->heap));
#endif
    }
}

void kipc_init(HANDLE handle, int size)
{
    PROCESS* process = (PROCESS*)handle;
    rb_init(&(process->kipc.rb), size);
    process->kipc.wait_process = (unsigned int)-1;
}

void kipc_post(IPC* ipc)
{
    kipc_post_process(ipc, (HANDLE)kprocess_get_current());
}

void kipc_read(IPC* ipc, TIME* time, HANDLE wait_process)
{
    PROCESS* process = kprocess_get_current();
    kipc_read_process(process, ipc, time, wait_process);
}

void kipc_call(IPC* ipc, TIME* time)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, ipc, sizeof(IPC));
    HANDLE wait_process = (HANDLE)(ipc->process);
    kipc_post(ipc);
    kipc_read_process(process, ipc, time, wait_process);
}
