/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "sys_config.h"
#include "can.h"

#ifdef EXODRIVERS
void can_open(unsigned int baudrate)
{
    ipc_post_exo(HAL_REQ(HAL_CAN, IPC_OPEN), baudrate, 0, 0);
}

void can_close()
{
    ipc_post_exo(HAL_REQ(HAL_CAN, IPC_CLOSE), 0, 0, 0);
}

void can_set_baudrate(int baudrate)
{
    ipc_post_exo(HAL_REQ(HAL_CAN, IPC_CAN_SET_BAUDRATE), baudrate, 0, 0);
}

void can_clear_error()
{
    ipc_post_exo(HAL_REQ(HAL_CAN, IPC_CAN_CLEAR_ERROR), 0, 0, 0);
}
#else

void can_open(unsigned int baudrate)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_CAN, IPC_OPEN), baudrate, 0, 0);
}

void can_close()
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_CAN, IPC_CLOSE), 0, 0, 0);
}

void can_set_baudrate(int baudrate)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_CAN, IPC_CAN_SET_BAUDRATE), baudrate, 0, 0);
}

void can_clear_error()
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_CAN, IPC_CAN_CLEAR_ERROR), 0, 0, 0);
}

#endif // EXODRIVERS


void can_state_decode(IPC* ipc, CAN_STATE_t* res)
{
    res->error = ipc->param2;
    res->state = ipc->param3;
}

void can_decode(IPC* ipc, CAN_MSG* msg)
{
    msg->id = ipc->param1 >> 21;
    msg->data_len = ipc->param1 & 0x0f;
    msg->rtr = (ipc->param1 & 0x10);
    msg->data.hi = ipc->param2;
    msg->data.lo = ipc->param3;
}
#ifdef EXODRIVERS
#define __CORE KERNEL_HANDLE
#else
#define __CORE object_get(SYS_OBJ_CORE)
#endif //EXODRIVERS
CAN_ERROR can_write(CAN_MSG* msg, CAN_STATE_t* res)
{
    IPC ipc;
    HANDLE process = __CORE;
    ipc.process = process;
    ipc.cmd = HAL_REQ(HAL_CAN, IPC_WRITE);
    ipc.param1 = (msg->id << 21) | (msg->data_len & 0x0f);
    if (msg->rtr)
        ipc.param1 |= 0x10;
    ipc.param2 = msg->data.hi;
    ipc.param3 = msg->data.lo;
    call(&ipc);
    res->error = ipc.param2;
    res->state = ipc.param3;
    return ipc.param2;
}

void can_get_state(CAN_STATE_t* res)
{
    IPC ipc;
    HANDLE process = __CORE;
    ipc.process = process;
    ipc.cmd = HAL_REQ(HAL_CAN, IPC_CAN_GET_STATE);
    call(&ipc);
    res->error = ipc.param2;
    res->state = ipc.param3;
}
