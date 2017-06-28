/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include "ipc.h"
#include "object.h"

typedef enum {
    IPC_CAN_SET_BAUDRATE = IPC_USER,
    IPC_CAN_BUS_ERROR,
    IPC_CAN_TXC,
    IPC_CAN_GET_STATE,
    IPC_CAN_CLEAR_ERROR
} CAN_IPCS;

typedef enum {
    OK = 0,
    BUSY,
    ILLEGAL_STATE,
    ARBITRATION_LOST,
    BUSOFF_ABORT,
    NO_CONFIG = ERROR_NOT_CONFIGURED
} CAN_ERROR;

typedef enum {
    NO_INIT = 0,
    INIT,
    BUSOFF,
    CAN_TRANSMIT, // dummy state only for compare
    PASSIVE_ERR,
    NORMAL
} CAN_STATE;

#pragma pack(push,1)
typedef struct {
    uint32_t id;
    uint32_t data_len;
    union {
        struct {
            uint8_t b0;
            uint8_t b1;
            uint8_t b2;
            uint8_t b3;
            uint8_t b4;
            uint8_t b5;
            uint8_t b6;
            uint8_t b7;
        };
        struct {
            uint16_t w0;
            uint16_t w1;
            uint16_t w2;
            uint16_t w3;
        };
        struct {
            uint32_t lo;
            uint32_t hi;
        };
    } data;
    bool rtr;
} CAN_MSG;
#pragma pack(pop)

typedef struct {
    CAN_STATE state;
    CAN_ERROR error;
} CAN_STATE_t;

void can_open(unsigned int baudrate);
void can_close();
void can_set_baudrate(int baudrate);
void can_clear_error();
void can_state_decode(IPC* ipc, CAN_STATE_t* res);
void can_decode(IPC* ipc, CAN_MSG* msg);
void can_get_state(CAN_STATE_t* res);
CAN_ERROR can_write(CAN_MSG* msg, CAN_STATE_t* res);

#endif // CAN_H
