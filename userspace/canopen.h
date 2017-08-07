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
#define CO_OD_MAX_ENTRY               50
#define CO_MAX_RPDO                   4
#define CO_MAX_TPDO                   4

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
#define OD_RO  (0UL << 31)
#define OD_RW  (1UL << 31)
#define CO_OD_IS_RW(od)  (od->len & OD_RW)

#define CO_OD_ENTRY_HEARTBEAT_TIME     0x001017

#define CO_OD_IDX(index, subindex) ((index) | (subindex << 16))
#define CO_OD_ADD(index,subindex,rw_mode,len,data) { {{index,subindex}},len|rw_mode,data}
#define CO_OD_END  { {{0, 0}},0, 0}
typedef struct {
    union {
        struct {
            uint16_t index;
            uint8_t subindex;
        };
        uint32_t idx;
    };
    uint32_t len;
    uint32_t data;
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
