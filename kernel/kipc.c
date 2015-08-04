/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg.h"
#include "kipc.h"
#include "kio.h"
#include "kprocess.h"
#include "../userspace/error.h"
#include "../userspace/rb.h"
#include <string.h>
#include "kernel.h"

#define IPC_ITEM(kprocess, num)              ((IPC*)((unsigned int)(kprocess->process) + sizeof(PROCESS) + (num) * sizeof(IPC)))

#define ANY_CMD                             0xffffffff

void kipc_lock_release(KPROCESS* kprocess)
{
    kprocess->kipc.wait_process = (KPROCESS*)INVALID_HANDLE;
}

static inline int kipc_index(KPROCESS* kprocess, HANDLE wait_process, unsigned int cmd)
{
    int i;
    unsigned int head = kprocess->process->ipcs.head;
    for (i = kprocess->process->ipcs.tail; i != head; i = RB_ROUND(&kprocess->process->ipcs, i + 1))
        if (((IPC_ITEM(kprocess, i)->process == wait_process) || (wait_process == ANY_HANDLE)) && ((IPC_ITEM(kprocess, i)->cmd == cmd) || (cmd == ANY_CMD)))
            return i;
    return -1;
}

void kipc_peek(KPROCESS *kprocess, int index, IPC* ipc)
{
    IPC tmp;
    for(; index != kprocess->process->ipcs.tail; index = RB_ROUND_BACK(&kprocess->process->ipcs, index - 1))
    {
        //swap
        memcpy(&tmp, IPC_ITEM(kprocess, index), sizeof(IPC));
        memcpy(IPC_ITEM(kprocess, index), IPC_ITEM(kprocess, RB_ROUND_BACK(&kprocess->process->ipcs, index - 1)), sizeof(IPC));
        memcpy(IPC_ITEM(kprocess, RB_ROUND_BACK(&kprocess->process->ipcs, index - 1)), &tmp, sizeof(IPC));
    }
    memcpy(ipc, IPC_ITEM(kprocess, kprocess->process->ipcs.tail), sizeof(IPC));
    rb_get(&kprocess->process->ipcs);
}

void kipc_read_process(KPROCESS *kprocess, IPC* ipc, KPROCESS* wait_process, unsigned int cmd)
{
    int index;
    if (wait_process == kprocess)
    {
        kprocess_error(kprocess, ERROR_DEADLOCK);
#if (KERNEL_IPC_DEBUG)
        printk("IPC deadlock: %s\n", kprocess_name(kprocess));
#endif //KERNEL_IPC_DEBUG
        return;
    }
    kprocess->kipc.ipc = ipc;
    kprocess_sleep(kprocess, NULL, PROCESS_SYNC_IPC, kprocess);

    disable_interrupts();
    if ((index = kipc_index(kprocess, (HANDLE)wait_process, cmd)) < 0)
    {
        kprocess->kipc.wait_process = wait_process;
        kprocess->kipc.cmd = cmd;
    }
    enable_interrupts();

    //maybe already on queue? Peek.
    if (index >= 0)
    {
        kprocess_wakeup(kprocess);
        kipc_peek(kprocess, index, ipc);
    }
}

void kipc_post_process(IPC* ipc, KPROCESS* sender)
{
    KPROCESS* receiver = (KPROCESS*)ipc->process;
    int index = -1;
    bool wake = false;
    IPC* cur;
    CHECK_HANDLE(receiver, sizeof(KPROCESS));
    CHECK_MAGIC(receiver, MAGIC_PROCESS);
    if (ipc->cmd & HAL_IO_FLAG)
    {
        if (!kio_send((IO*)ipc->param2, receiver))
            return;
    }
    disable_interrupts();
    if ((wake = ((receiver->kipc.wait_process == sender || receiver->kipc.wait_process == (KPROCESS*)ANY_HANDLE) &&
                 (receiver->kipc.cmd == ipc->cmd || receiver->kipc.cmd == ANY_CMD))))
    {
        receiver->kipc.wait_process = (KPROCESS*)INVALID_HANDLE;
        receiver->kipc.cmd = ANY_CMD;
    }
    if (!rb_is_full(&receiver->process->ipcs))
        index = rb_put(&receiver->process->ipcs);
    enable_interrupts();
    if (index >= 0)
    {
        cur = IPC_ITEM(receiver, index);
        cur->cmd = ipc->cmd;
        cur->param1 = ipc->param1;
        cur->param2 = ipc->param2;
        cur->param3 = ipc->param3;
        cur->process = (HANDLE)sender;
        //already waiting?
        if (wake)
        {
            kprocess_wakeup(receiver);
            kipc_peek(receiver, index, receiver->kipc.ipc);
        }
    }
    else
    {
        kprocess_error(sender, ERROR_OVERFLOW);
#if (KERNEL_IPC_DEBUG)
        printk("Error: receiver %s IPC overflow!\n", kprocess_name(receiver));
        if (sender == (KPROCESS*)KERNEL_HANDLE)
            printk("Sender: kernel\n");
        else
            printk("Sender: %s\n", kprocess_name((KPROCESS*)sender));
        printk("cmd: %#X, p1: %#X, p2: %#X, p3: %#X\n", ipc->cmd, ipc->param1, ipc->param2, ipc->param3);
#if (KERNEL_DEVELOPER_MODE)
        HALT();
#endif
#endif
    }
}

void kipc_init(KPROCESS *kprocess)
{
    rb_init(&(kprocess->process->ipcs), KERNEL_IPC_COUNT);
    kprocess->kipc.wait_process = (KPROCESS*)INVALID_HANDLE;
    kprocess->kipc.cmd = ANY_CMD;
}

void kipc_post(IPC* ipc)
{
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_ADDRESS(kprocess, ipc, sizeof(IPC));
    kipc_post_process(ipc, kprocess);
}

void kipc_read(IPC* ipc, KPROCESS* wait_process)
{
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_ADDRESS(kprocess, ipc, sizeof(IPC));
    kipc_read_process(kprocess, ipc, wait_process, ANY_CMD);
}

void kipc_call(IPC* ipc)
{
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_ADDRESS(kprocess, ipc, sizeof(IPC));
    KPROCESS* wait_process = (KPROCESS*)(ipc->process);
    kipc_post_process(ipc, kprocess);
    kipc_read_process(kprocess, ipc, wait_process, ipc->cmd);
}
