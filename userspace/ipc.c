/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ipc.h"
#include "rb.h"
#include "process.h"
#include "svc.h"
#include "error.h"
#include <string.h>

#define IPC_ITEM(num)                           ((IPC*)((unsigned int)(__GLOBAL->process) + sizeof(PROCESS) + (num) * sizeof(IPC)))

static int ipc_index(HANDLE wait_process, unsigned int cmd, unsigned int param1)
{
    int i;
    unsigned int head = __GLOBAL->process->ipcs.head;
    for (i = __GLOBAL->process->ipcs.tail; i != head; i = RB_ROUND(&__GLOBAL->process->ipcs, i + 1))
        if (((IPC_ITEM(i)->process == wait_process) || (wait_process == ANY_HANDLE)) && ((IPC_ITEM(i)->cmd == cmd) || (cmd == ANY_CMD)) &&
             ((IPC_ITEM(i)->param1 == param1) || (param1 == ANY_HANDLE)))
            return i;
    return -1;
}

static IPC* ipc_peek(int index, IPC* ipc)
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

unsigned int ipc_remove(HANDLE process, unsigned int cmd, unsigned int param1)
{
    unsigned int count;
    int index;
    IPC tmp;
    for(count = 0; (index = ipc_index(process, cmd, param1)) >= 0; ++count)
        ipc_peek(index, &tmp);
    return count;
}

void ipc_post(IPC* ipc)
{
    svc_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
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
        if (rb_is_empty(&__GLOBAL->process->ipcs))
            svc_call(SVC_IPC_WAIT, ANY_HANDLE, ANY_CMD, ANY_HANDLE);
        ipc_peek(__GLOBAL->process->ipcs.tail, ipc);
        if (ipc->cmd == HAL_REQ(HAL_SYSTEM, IPC_PING))
            ipc_write(ipc);
        else
            break;
    }
}

void ipc_read_ex(IPC* ipc, HANDLE process, unsigned int cmd, unsigned int param1)
{
    if (ipc_index(process, cmd, param1) < 0)
        svc_call(SVC_IPC_WAIT, process, cmd, param1);
    ipc_peek(ipc_index(process, cmd, param1), ipc);
}

void ipc_write(IPC* ipc)
{
    if (ipc->cmd & HAL_REQ_FLAG)
    {
        ipc->cmd &= ~HAL_REQ_FLAG;
        switch (get_last_error())
        {
        case ERROR_OK:
            //no error
            break;
        case ERROR_SYNC:
            //asyncronous completion
            return;
        default:
            ipc->param3 = get_last_error();
        }
        svc_call(SVC_IPC_POST, (unsigned int)ipc, 0, 0);
    }
}

void call(IPC* ipc)
{
    svc_call(SVC_IPC_CALL, (unsigned int)ipc, 0, 0);
    ipc_peek(ipc_index(ipc->process, ipc->cmd & ~HAL_REQ_FLAG, ipc->param1), ipc);
}

void ack(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    call(&ipc);
}

unsigned int get(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    call(&ipc);
    if ((int)(ipc.param3) < 0)
        error(ipc.param3);
    return ipc.param2;
}

unsigned int get_handle(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.process = process;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    call(&ipc);
    if (ipc.param2 == INVALID_HANDLE)
        error(ipc.param3);
    return ipc.param2;
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
    if ((int)(ipc.param3) < 0)
        error(ipc.param3);
    return (int)(ipc.param3);
}
