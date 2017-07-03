/*
 RExOS - embedded RTOS
 Copyright (c) 2011-2017, Alexey Kramarenko
 All rights reserved.
 */

#include "sys_config.h"
#include "object.h"
#include "canopen.h"

extern void canopens_main();

void canopen_fault(HANDLE co, uint32_t err_code)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CO_FAULT), 0, err_code, 0);
}

void canopen_lss_find(HANDLE co, uint32_t bit_cnt, uint32_t lss_id)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CO_LSS_FIND), 0, bit_cnt, lss_id);

}
void canopen_lss_set_id(HANDLE co, uint32_t id)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CO_LSS_SET_ID), id, 0, 0);
}

HANDLE canopen_create(uint32_t process_size, uint32_t priority)
{
    char name[] = "CANopen Server";
    REX rex;
    rex.name = name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = canopens_main;
    return process_create(&rex);
}

bool canopen_open(HANDLE co, uint32_t baudrate, uint32_t lss_id, uint32_t id)
{
    return get_handle(co, HAL_REQ(HAL_CANOPEN, IPC_OPEN), baudrate, lss_id, id) != INVALID_HANDLE;
}

void canopen_close(HANDLE co)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CLOSE), 0, 0, 0);
}
