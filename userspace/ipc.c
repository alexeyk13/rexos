/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "ipc.h"
#include "rb.h"
#include "process.h"
#include "svc.h"
#include "error.h"
#include <string.h>

#define IPC_ITEM(num)                           ((IPC*)((unsigned int)(__GLOBAL->process) + sizeof(PROCESS) + (num) * sizeof(IPC)))

static inline int ipc_index(HANDLE wait_process, unsigned int cmd, unsigned int param1)
{
    int i;
    unsigned int head = __GLOBAL->process->ipcs.head;
    for (i = __GLOBAL->process->ipcs.tail; i != head; i = RB_ROUND(&__GLOBAL->process->ipcs, i + 1))
        if (((IPC_ITEM(i)->process == wait_process) || (wait_process == ANY_HANDLE)) && ((IPC_ITEM(i)->cmd == cmd) || (cmd == ANY_CMD)) &&
             ((IPC_ITEM(i)->param1 == param1) || (param1 == ANY_HANDLE)))
            return i;
    return -1;
}

IPC* ipc_peek(int index, IPC* ipc)
{
    //cmd, process, handle
    IPC tmp;
    for(; index != __GLOBAL->process->ipcs.tail; index = RB_ROUND_BACK(&__GLOBAL->process->ipcs, index - 1))
    {
        //swap
        memcpy(&tmp, IPC_ITEM(index), sizeof(IPC));
        memcpy(IPC_ITEM(index), IPC_ITEM(RB_ROUND_BACK(&__GLOBAL->process->ipcs, index - 1)), sizeof(IPC));
        memcpy(IPC_ITEM(RB_ROUND_BACK(&__GLOBAL->process->ipcs, index - 1)), &tmp, sizeof(IPC));
    }
    memcpy(ipc, IPC_ITEM(__GLOBAL->process->ipcs.tail), sizeof(IPC));
    rb_get(&__GLOBAL->process->ipcs);
    return ipc;
}

void ipc_post(IPC* ipc)
{
    svc_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
}

void ipc_post_ex(IPC* ipc, int param3)
{
    ipc->param3 = param3;
    ipc_post(ipc);
}

void ipc_post_inline(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.process = process;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    svc_call(SVC_IPC_POST, (unsigned int)&ipc, 0, 0);
}

void ipc_post_or_error(IPC* ipc)
{
    ipc->param3 = get_last_error();
    svc_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
}

void ipc_ipost(IPC* ipc)
{
    __GLOBAL->svc_irq(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
}

void ipc_ipost_inline(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.process = process;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    ipc_ipost(&ipc);
}

void ipc_read(IPC* ipc)
{
    for (;;)
    {
        error(ERROR_OK);
        svc_call(SVC_IPC_WAIT, ANY_HANDLE, ANY_CMD, 0);
        ipc_peek(__GLOBAL->process->ipcs.tail, ipc);
        if (ipc->cmd == HAL_CMD(HAL_SYSTEM, IPC_PING))
            ipc_post_or_error(ipc);
        else
            break;
    }
}

bool call(IPC* ipc)
{
    svc_call(SVC_IPC_CALL, (unsigned int)ipc, 0, 0);
    ipc_peek(ipc_index(ipc->process, ipc->cmd, ipc->param1), ipc);

    if (ipc->param3 >= 0)
        return true;
    error(ipc->param3);
    return false;
}

bool ack(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    return call(&ipc);
}

unsigned int get(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    if (call(&ipc))
        return ipc.param2;
    return INVALID_HANDLE;
}

int get_size(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    call(&ipc);
    return (int)ipc.param3;
}
