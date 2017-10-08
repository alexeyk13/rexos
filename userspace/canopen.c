/*
 RExOS - embedded RTOS
 Copyright (c) 2011-2017, Alexey Kramarenko
 All rights reserved.
 */

#include "sys_config.h"
#include "object.h"
#include "canopen.h"
#include "io.h"
#include <string.h>

extern void canopens_main();

void canopen_fault(HANDLE co, uint32_t err_code)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CO_FAULT), 0, err_code, 0);
}

void canopen_lss_find(HANDLE co, LSS_FIND* lss)
{
    IO* io;
    io = io_create(sizeof(LSS_FIND));
    memcpy(io_data(io), lss, sizeof(LSS_FIND));
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CO_LSS_FIND), 0, (uint32_t)io, 0);
    io_destroy(io);

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
void canopen_send_pdo(HANDLE co, uint8_t pdo_num, uint8_t pdo_len, uint32_t hi, uint32_t lo)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CO_SEND_PDO), pdo_num | (pdo_len << 16), hi, lo);
}

void canopen_od_change(HANDLE co, uint32_t idx, uint32_t data)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CO_OD_CHANGE), idx, data, 0);
}

void canopen_open(HANDLE co, IO* od, uint32_t baudrate, uint8_t id)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_OPEN), baudrate, (uint32_t)od, id);
}

void canopen_close(HANDLE co)
{
    ack(co, HAL_REQ(HAL_CANOPEN, IPC_CLOSE), 0, 0, 0);
}

void canopen_sdo_init_req(HANDLE co, CO_SDO_REQ* req)
{
    IPC ipc;
    ipc.process = co;
    ipc.cmd = HAL_CMD(HAL_CANOPEN, IPC_CO_SDO_REQUEST);
    ipc.param1 = (req->id << 24) | (req->subindex << 16) | req->index;
    ipc.param2 = req->type;
    ipc.param3 = req->data;
    ipc_post(&ipc);
}

//------------------- Object Dictionary --------------------
bool co_od_write(CO_OD_ENTRY* od, uint16_t index, uint8_t subindex, uint32_t value)
{
    uint32_t i;
    for (i = 0; i < CO_OD_MAX_ENTRY; i++, od++)
    {
        if (od->idx == 0)
            return false;
        if (od->idx != CO_OD_IDX(index, subindex))
            continue;
        od->data = value;
        return true;
    }
    return false;
}
uint32_t co_od_get_u32(CO_OD_ENTRY* od, uint16_t index, uint8_t subindex)
{

    do
    {
        if (od->idx != CO_OD_IDX(index, subindex))
            continue;
        return od->data;
    } while ((++od)->idx);
    return 0;
}

CO_OD_ENTRY* co_od_find(CO_OD_ENTRY* od, uint16_t index, uint8_t subindex)
{
    uint32_t i;
    for (i = 0; i < CO_OD_MAX_ENTRY; i++, od++)
    {
        if (od->idx == 0)
            return NULL;
        if (od->idx == CO_OD_IDX(index, subindex))
            return od;
    }
    return NULL;
}

CO_OD_ENTRY* co_od_find_idx(CO_OD_ENTRY* od, uint32_t idx)
{
    uint32_t i;
    for (i = 0; i < CO_OD_MAX_ENTRY; i++, od++)
    {
        if (od->idx == 0)
            return NULL;
        if (od->idx == idx)
            return od;
    }
    return NULL;
}
uint32_t co_od_size(CO_OD_ENTRY* od)
{
    uint32_t i;
    for (i = 0; i < CO_OD_MAX_ENTRY; i++, od++)
    {
        if (od == NULL)
            return 0;
        if (od->idx == 0)
            return i+1;
    }
    return 0;
}

