/*
 RExOS - embedded RTOS
 Copyright (c) 2011-2017, Alexey Kramarenko
 All rights reserved.
 */

#ifndef CANOPEN_H
#define CANOPEN_H

#include <stdint.h>
#include "object.h"
#include "ipc.h"

typedef enum {
    BUS_RESET = 0,
    BUS_INIT,
    BUS_ERROR,
    BUS_RUN
} BUS_STATE;

typedef enum {
    LED_OPERATE = 0,
    LED_CONFIG,
    LED_NO_CONFIG,
    LED_BUSOFF,
    LED_LOST,
    LED_INIT
} LED_STATE;

typedef enum {
    IPC_CO_OPEN = IPC_USER,
    IPC_CO_CHANGE_STATE,
    IPC_CO_FAULT,
    IPC_CO_PDO,
    IPC_CO_INIT,
    IPC_CO_LSS_FIND,
    IPC_CO_LSS_FOUND,
    IPC_CO_LSS_SET_ID,
    IPC_CO_LED_STATE,
    IPC_CO_CLEAR_ERROR
} CANOPEN_IPCS;

void canopen_fault(HANDLE co, uint32_t err_code);
void canopen_lss_find(HANDLE co, uint32_t bit_cnt, uint32_t lss_id);
void canopen_lss_set_id(HANDLE co, uint32_t id);
HANDLE canopen_create(uint32_t process_size, uint32_t priority);
bool canopen_open(HANDLE co, uint32_t baudrate, uint32_t lss_id, uint32_t id);
void canopen_close(HANDLE co);

#endif // CANOPEN_H
