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
#include "kerror.h"
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

static bool kipc_send(HANDLE sender, HANDLE receiver, unsigned int cmd, void* param)
{
    bool res = true;
    switch (cmd & HAL_MODE)
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

static void kipc_post_internal(HANDLE sender, HANDLE receiver, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC* cur;
    KPROCESS* r;
    int index;
    index = -1;
    r = (KPROCESS*)receiver;
    disable_interrupts();
    if (!rb_is_full(&r->process->ipcs))
        index = rb_put(&r->process->ipcs);
    enable_interrupts();
    if (index >= 0)
    {
        cur = KIPC_ITEM(r, index);
        cur->cmd = cmd;
        cur->param1 = param1;
        cur->param2 = param2;
        cur->param3 = param3;
        cur->process = sender;
    }
    else
    {
        error(ERROR_OVERFLOW);
#if (KERNEL_IPC_DEBUG)
        printk("Error: receiver %s IPC overflow!\n", kprocess_name((HANDLE)receiver));
        printk("Sender: ");
        if (sender == KERNEL_HANDLE)
            printk("Kernel\n");
        else
            printk("%s\n", kprocess_name(sender));
        printk("cmd: %#X, p1: %#X, p2: %#X, p3: %#X\n", cmd, param1, param2, param3);
#if (KERNEL_DEVELOPER_MODE)
        HALT();
#endif //KERNEL_DEVELOPER_MODE
#endif //KERNEL_IPC_DEBUG
    }
}

void kipc_post(HANDLE sender, IPC* ipc)
{
    KPROCESS* receiver;
    CHECK_MAGIC((KPROCESS*)ipc->process, MAGIC_PROCESS);

    if (!kipc_send(sender, ipc->process, ipc->cmd, (void*)ipc->param2))
    {
        //can't be delivered. Return response back with error (if required)
        if (ipc->cmd & HAL_REQ_FLAG)
            kipc_post_internal(ipc->process, sender, ipc->cmd & ~HAL_REQ_FLAG, ipc->param1, ipc->param2, get_last_error());
        return;
    }
#ifdef EXODRIVERS
    if (ipc->process == KERNEL_HANDLE)
    {
        int old_kerror = kget_last_error();
        ipc->process = sender;
        exodriver_post(ipc);
        ipc->process = KERNEL_HANDLE;
        //send back if response is received
        if ((ipc->cmd & HAL_REQ_FLAG) && (kget_last_error() != ERROR_SYNC))
        {
            kipc_send(KERNEL_HANDLE, sender, ipc->cmd, (void*)ipc->param2);

            if (kget_last_error() != ERROR_OK)
                ipc->param3 = kget_last_error();
            kipc_post_internal(ipc->process, sender, ipc->cmd & ~HAL_REQ_FLAG, ipc->param1, ipc->param2, ipc->param3);
        }
        kerror(old_kerror);
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
    enable_interrupts();
    kipc_post_internal(sender, ipc->process, ipc->cmd, ipc->param1, ipc->param2, ipc->param3);
}

void kipc_post_exo(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    ipc.process = process;
    kipc_post(KERNEL_HANDLE, &ipc);
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
