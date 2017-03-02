/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg.h"
#include "kipc.h"
#include "kio.h"
#include "kprocess.h"
#include "kheap.h"
#include "kprocess_private.h"
#include "../userspace/error.h"
#include "../userspace/rb.h"
#include "../userspace/core/core.h"
#include "kernel.h"
#include "kernel_config.h"

#define KIPC_ITEM(p, num)                               ((IPC*)((unsigned int)(((KPROCESS*)(p))->process) + sizeof(PROCESS) + (num) * sizeof(IPC)))

void kipc_init(KPROCESS *process)
{
    rb_init(&(process->process->ipcs), KERNEL_IPC_COUNT);
    process->kipc.wait_process = INVALID_HANDLE;
    process->kipc.cmd = ANY_CMD;
}

void kipc_lock_release(KPROCESS* process)
{
    process->kipc.wait_process = INVALID_HANDLE;
}

static inline int kipc_index(HANDLE p, HANDLE wait_process, unsigned int cmd, unsigned int param1)
{
    KPROCESS* process;
    int i;
    unsigned int head;
    process = (KPROCESS*)p;
    head = process->process->ipcs.head;
    for (i = process->process->ipcs.tail; i != head; i = RB_ROUND(&process->process->ipcs, i + 1))
        if (((KIPC_ITEM(process, i)->process == wait_process) || (wait_process == ANY_HANDLE)) && ((KIPC_ITEM(process, i)->cmd == cmd) || (cmd == ANY_CMD)) &&
                ((KIPC_ITEM(process, i)->param1 == param1) || (param1 == ANY_HANDLE)))
            return i;
    return -1;
}

#if (KERNEL_IPC_DEBUG)
static void kipc_debug_overflow(IPC* ipc, HANDLE sender)
{
    printk("Error: receiver %s IPC overflow!\n", kprocess_name(ipc->process));
    printk("Sender: ");
    if (sender == KERNEL_HANDLE)
        printk("Kernel\n");
    else
        printk("%s\n", kprocess_name(sender));
    printk("cmd: %#X, p1: %#X, p2: %#X, p3: %#X\n", ipc->cmd, ipc->param1, ipc->param2, ipc->param3);
#if (KERNEL_DEVELOPER_MODE)
    HALT();
#endif //KERNEL_DEVELOPER_MODE
}
#endif //KERNEL_IPC_DEBUG

static bool kipc_send(HANDLE sender, HANDLE receiver, void* param)
{
    bool res = true;
    switch (ipc->cmd & HAL_MODE)
    {
    case HAL_IO_MODE:
        res = kio_send(sender, param, receiver);
        break;
#if (KERNEL_HEAP)
    case HAL_HEAP_MODE:
        kheap_send(param, receiver);
        break;
#endif //KERNEL_HEAP
    default:
        break;
    }
    return res;
}

void kipc_post(HANDLE sender, IPC* ipc)
{
    KPROCESS* receiver;
    int index;
    IPC* cur;
    index = -1;
    CHECK_MAGIC(ipc->process, MAGIC_PROCESS);
    if (!kipc_send(sender, ipc->process, (void*)ipc->param2))
        return;
#ifdef EXODRIVERS
    int err;
    if (ipc->process == KERNEL_HANDLE)
    {
        ipc->process = sender;
        error(ERROR_OK);
        exodriver_post(ipc);
        err = get_last_error();
        if ((ipc->cmd & HAL_REQ_FLAG) && (err != ERROR_SYNC))
        {
            //send back if response is received
            if (!kipc_send(ipc->process, sender, (void*)ipc->param2))
                return;
            
            //send response ipc inline
            if (err != ERROR_OK)
                ipc->param3 = err;
            disable_interrupts();
            if (!rb_is_full(&((KPROCESS*)sender)->process->ipcs))
                index = rb_put(&((KPROCESS*)sender)->process->ipcs);
            enable_interrupts();
            if (index >= 0)
            {
                cur = KIPC_ITEM(sender, index);
                cur->cmd = ipc->cmd & ~HAL_REQ_FLAG;
                cur->param1 = ipc->param1;
                cur->param2 = ipc->param2;
                cur->param3 = (err != ERROR_OK ? err : ipc->param3);
                cur->process = KERNEL_HANDLE;
            }
#if (KERNEL_IPC_DEBUG)
            else
                kipc_debug_overflow(ipc, sender);
#endif //KERNEL_IPC_DEBUG
        }
        ipc->process = KERNEL_HANDLE;
        return;
    }
#endif //EXODRIVERS

    receiver = (KPROCESS*)ipc->process;
    disable_interrupts();
    if ((receiver->kipc.wait_process == sender || receiver->kipc.wait_process == ANY_HANDLE) &&
                 (receiver->kipc.cmd == ipc->cmd || receiver->kipc.cmd == ANY_CMD) &&
                 ((receiver->kipc.param1 == ipc->param1) || (receiver->kipc.param1 == ANY_HANDLE)))
    {
        //already waiting? Wakeup him
        receiver->kipc.wait_process = INVALID_HANDLE;
        kprocess_wakeup((HANDLE)receiver);
    }
    if (!rb_is_full(&receiver->process->ipcs))
        index = rb_put(&receiver->process->ipcs);
    enable_interrupts();
    if (index >= 0)
    {
        cur = KIPC_ITEM(receiver, index);
        cur->cmd = ipc->cmd;
        cur->param1 = ipc->param1;
        cur->param2 = ipc->param2;
        cur->param3 = ipc->param3;
        cur->process = (HANDLE)sender;
    }
    else
    {
        error(ERROR_OVERFLOW);
#if (KERNEL_IPC_DEBUG)
        kipc_debug_overflow(ipc, sender);
#endif //KERNEL_IPC_DEBUG
    }
}

void kipc_wait(HANDLE process, HANDLE wait_process, unsigned int cmd, unsigned int param1)
{
    if (wait_process == process)
    {
#if (KERNEL_IPC_DEBUG)
        printk("IPC deadlock: %s\n", kprocess_name(process));
#endif //KERNEL_IPC_DEBUG
        error(ERROR_DEADLOCK);
        return;
    }
    kprocess_sleep(process, NULL, PROCESS_SYNC_IPC, INVALID_HANDLE);

    disable_interrupts();
    if (kipc_index(process, wait_process, cmd, param1) >= 0)
        //maybe already on queue? Wakeup process
        kprocess_wakeup(process);
    else
    {
        ((KPROCESS*)process)->kipc.wait_process = wait_process;
        ((KPROCESS*)process)->kipc.cmd = cmd;
        ((KPROCESS*)process)->kipc.param1 = param1;
    }
    enable_interrupts();

}

void kipc_call(HANDLE process, IPC* ipc)
{
    kipc_post(process, ipc);
    kipc_wait(process, ipc->process, ipc->cmd & ~HAL_REQ_FLAG, ipc->param1);
}
