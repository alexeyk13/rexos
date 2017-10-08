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
#include "io.h"

#define LSS_CHECK_ALL_BITS            31
#define CO_OD_MAX_ENTRY               100
#define CO_MAX_RPDO                   4
#define CO_MAX_TPDO                   8

#define PDO1_TX                       (0x3 << 7)
#define PDO1_RX                       (0x4 << 7)
#define PDO2_TX                       (0x5 << 7)
#define PDO2_RX                       (0x6 << 7)
#define PDO3_TX                       (0x7 << 7)
#define PDO3_RX                       (0x8 << 7)
#define PDO4_TX                       (0x9 << 7)
#define PDO4_RX                       (0xA << 7)

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
    IPC_CO_INIT,
    IPC_CO_OD_CHANGE,
    IPC_CO_LED_STATE,
    IPC_CO_CLEAR_ERROR,
    IPC_CO_ID_CHANGED,
    IPC_CO_SAVE_OD,
    IPC_CO_RESTORE_OD,
    IPC_CO_GET_OD,
    IPC_CO_HARDW_RESET,
// only master ipc
    IPC_CO_SEND_PDO,
    IPC_CO_LSS_FIND,
    IPC_CO_LSS_FOUND,
    IPC_CO_LSS_SET_ID,
    IPC_CO_SDO_REQUEST,
    IPC_CO_SDO_RESULT

} CANOPEN_IPCS;

typedef struct {
    union {
        uint32_t data[4];
        struct {
            uint32_t vendor;
            uint32_t product;
            uint32_t version;
            uint32_t serial;
        };
    };
} CO_IDENTITY;

//------ LSS ------
typedef enum {
    LSS_VENDOR = 0,
    LSS_PRODUCT,
    LSS_VERSION,
    LSS_SERIAL
} LSS_POS;

typedef struct {
    CO_IDENTITY lss_addr;
    union {
        uint8_t pos[4];
        struct {
            uint8_t vendor_bits;
            uint8_t product_bits;
            uint8_t version_bits;
            uint8_t serial_bits;
        };
    };
    uint8_t start_pos;
} LSS_FIND;
// --- object dictionary entry ------
#define OD_RO                               (0UL << 31)
#define OD_RW                               (1UL << 31)
#define CO_OD_IS_RW(od)                     (od->len & OD_RW)

#define CO_OD_SUBINDEX_Pos                  16
#define CO_OD_IDX(index, subindex)          ( (index) | ((subindex) << CO_OD_SUBINDEX_Pos) )
#define CO_OD_INDEX(idx)                    ( (idx) & (0xFFFF) )
#define CO_OD_SUBINDEX(idx)                 ( (idx) >> CO_OD_SUBINDEX_Pos)
#define CO_OD_ADD_FLOAT(index, subindex, rw_mode, len, d)   { {{(index), (subindex)}}, (len) | (rw_mode), {.f = (d)}}
#define CO_OD_ADD(index, subindex, rw_mode, len, d)   { {{(index), (subindex)}}, (len) | (rw_mode), {.data= (d)}}
#define CO_OD_END                                        { {{0, 0}}, 0, {.data = 0}}
#define CO_OD_FIRST_USER_INDEX               0x2000
#define CO_OD_MAX_INDEX                      0xC000

#define CO_OD_ENTRY_HEARTBEAT_TIME          CO_OD_IDX(0x1017, 0)
#define CO_OD_INDEX_SAVE_OD                 0x1010
#define CO_OD_INDEX_RESTORE_OD              0x1011


typedef struct {
    union {
        struct {
            uint16_t index;
            uint8_t subindex;
        };
        uint32_t idx;
    };
    uint32_t len;
    union {
        uint32_t data;
        float  f;
    };
//    uint32_t data;
} CO_OD_ENTRY;

typedef enum {
    CO_SDO_U8 = 0,
    CO_SDO_U16,
    CO_SDO_U32,
    CO_SDO_OK,
    CO_SDO_ERROR,
    CO_SDO_GET
} CO_SDO_TYPE;
typedef struct {
    uint8_t id;
    union {
        uint32_t idx;
        struct {
            uint16_t index;
            uint8_t subindex;
        };
    };
    CO_SDO_TYPE type;
    uint32_t data;
} CO_SDO_REQ;
//--------------------------------
void canopen_fault(HANDLE co, uint32_t err_code);
HANDLE canopen_create(uint32_t process_size, uint32_t priority);
void canopen_open(HANDLE co, IO* od, uint32_t baudrate, uint8_t id);
void canopen_close(HANDLE co);
void canopen_send_pdo(HANDLE co, uint8_t pdo_num, uint8_t pdo_len, uint32_t hi, uint32_t lo);
void canopen_od_change(HANDLE co, uint32_t idx, uint32_t data);

//--- only master -----
void canopen_lss_find(HANDLE co, LSS_FIND* lss);
void canopen_lss_set_id(HANDLE co, uint32_t id);
void canopen_sdo_init_req(HANDLE co, CO_SDO_REQ* req);

//--- object dictionary  ------
uint32_t co_od_get_u32(CO_OD_ENTRY* od, uint16_t index, uint8_t subindex);
uint32_t co_od_size(CO_OD_ENTRY* od);
bool co_od_write(CO_OD_ENTRY* od, uint16_t index, uint8_t subindex, uint32_t value);
CO_OD_ENTRY* co_od_find(CO_OD_ENTRY* od, uint16_t index, uint8_t subindex);
CO_OD_ENTRY* co_od_find_idx(CO_OD_ENTRY* od, uint32_t idx);

#endif // CANOPEN_H
