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

#define ANY_CMD                             0xffffffff

void kipc_lock_release(HANDLE process)
{
    ((PROCESS*)process)->kipc.wait_process = INVALID_HANDLE;
}

static inline int kipc_index(PROCESS* process, HANDLE wait_process, unsigned int cmd)
{
    int i;
    unsigned int head = process->kipc.rb.head;
    for (i = process->kipc.rb.tail; i != head; i = RB_ROUND(&process->kipc.rb, i + 1))
        if (((IPC_ITEM(process, i)->process == wait_process) || (wait_process == ANY_HANDLE)) && ((IPC_ITEM(process, i)->cmd == cmd) || (cmd == ANY_CMD)))
            return i;
    return -1;
}

void kipc_read_process(PROCESS* process, IPC* ipc, TIME* time, HANDLE wait_process, unsigned int cmd)
{
    int i;
    IPC tmp;
    CHECK_ADDRESS(process, time, sizeof(TIME));
    CHECK_ADDRESS(process, ipc, sizeof(IPC));
#if (KERNEL_IPC_DEBUG)
    if (wait_process == (HANDLE)process)
        printk("Warning: calling wait IPC with receiver same as caller can cause deadlock! process: %s\n\r", kprocess_name(process));
#endif
    process->kipc.ipc = ipc;
    kprocess_sleep(process, time, PROCESS_SYNC_IPC, process);

    disable_interrupts();
    if ((i = kipc_index(process, wait_process, cmd)) < 0)
    {
        process->kipc.wait_process = wait_process;
        process->kipc.cmd = cmd;
    }
    enable_interrupts();

    //maybe already on queue? Peek.
    if (i >= 0)
    {
        for(; i != process->kipc.rb.tail; i= RB_ROUND_BACK(&process->kipc.rb, i - 1))
        {
            //swap
            memcpy(&tmp, IPC_ITEM(process, i), sizeof(IPC));
            memcpy(IPC_ITEM(process, i), IPC_ITEM(process, RB_ROUND_BACK(&process->kipc.rb, i - 1)), sizeof(IPC));
            memcpy(IPC_ITEM(process, RB_ROUND_BACK(&process->kipc.rb, i - 1)), &tmp, sizeof(IPC));
        }
        memcpy(ipc, IPC_ITEM(process, process->kipc.rb.tail), sizeof(IPC));
        disable_interrupts();
        rb_get(&process->kipc.rb);
        enable_interrupts();
        kprocess_wakeup(process);
    }
}

void kipc_post_process(IPC* ipc, unsigned int sender)
{
    CHECK_ADDRESS(sender, ipc, sizeof(IPC));
    PROCESS* receiver = (PROCESS*)ipc->process;
    bool wake = false;
    IPC* cur = NULL;
    CHECK_HANDLE(receiver, sizeof(PROCESS));
    CHECK_MAGIC(receiver, MAGIC_PROCESS);
    disable_interrupts();
    if ((wake = ((receiver->kipc.wait_process == sender || receiver->kipc.wait_process == ANY_HANDLE) && (receiver->kipc.cmd == ipc->cmd || receiver->kipc.cmd == ANY_CMD))))
    {
        receiver->kipc.wait_process = INVALID_HANDLE;
        receiver->kipc.cmd = ANY_CMD;
    }
    else if (!rb_is_full(&receiver->kipc.rb))
        cur = IPC_ITEM(receiver, rb_put(&receiver->kipc.rb));
    enable_interrupts();
    //already waiting?
    if (wake)
    {
        receiver->kipc.ipc->cmd = ipc->cmd;
        receiver->kipc.ipc->param1 = ipc->param1;
        receiver->kipc.ipc->param2 = ipc->param2;
        receiver->kipc.ipc->param3 = ipc->param3;
        receiver->kipc.ipc->process = sender;
        kprocess_wakeup(receiver);
    }
    else if (cur)
    {
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
        printk("Error: receiver %s IPC overflow!\n\r", kprocess_name(receiver));
        if (sender == KERNEL_HANDLE)
            printk("Sender: kernel\n\r");
        else
            printk("Sender: %s\n\r", kprocess_name((PROCESS*)sender));
        printk("cmd: %#X, p1: %#X, p2: %#X, p3: %#X\n\r", ipc->cmd, ipc->param1, ipc->param2, ipc->param3);
#if (KERNEL_DEVELOPER_MODE)
        HALT();
#endif
#endif
    }
}

void kipc_init(HANDLE handle, int size)
{
    PROCESS* process = (PROCESS*)handle;
    //stub
    if (size == 0)
        size = 1;
    rb_init(&(process->kipc.rb), size);
    process->kipc.wait_process = INVALID_HANDLE;
    process->kipc.cmd = ANY_CMD;
}

void kipc_post(IPC* ipc)
{
    PROCESS* process = kprocess_get_current();
    kipc_post_process(ipc, (HANDLE)process);
}

void kipc_read(IPC* ipc, TIME* time, HANDLE wait_process)
{
    PROCESS* process = kprocess_get_current();
    kipc_read_process(process, ipc, time, wait_process, ANY_CMD);
}

void kipc_call(IPC* ipc, TIME* time)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, ipc, sizeof(IPC));
    HANDLE wait_process = (HANDLE)(ipc->process);
    kipc_post_process(ipc, (HANDLE)process);
    kipc_read_process(process, ipc, time, wait_process, ipc->cmd);
}
