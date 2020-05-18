/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef _NRF_RF_H_
#define _NRF_RF_H_

#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "../../userspace/nrf/nrf_driver.h"
#include "../../userspace/nrf/nrf_radio.h"
#include "sys_config.h"
#include "nrf_exo.h"
#include <stdbool.h>

#define REC_PDU_BUFFER_LEN              5

typedef enum {
    RADIO_STATE_IDLE = 0,
    RADIO_STATE_TX,
    RADIO_STATE_RX,
} RADIO_STATE;

#pragma pack(push,1)
typedef struct {
    BLE_ADV_PKT pkt;
    int8_t rssi;
    uint8_t ch;
} BLE_ADV_REC_PDU;
#pragma pack(pop)

typedef struct {
    uint32_t channel;
    uint16_t id;
} RADIO_DATA;

typedef struct {
    RADIO_STATE state;
    HANDLE process, timer;
    bool active;
    bool is_rx;
    RADIO_DATA curr;
    RADIO_DATA next;
//    BLE_MAC mac;

    // RX data
    void* param;
    IO* rx_io;
    unsigned int max_size;
    uint32_t rx_head;
    uint32_t rx_tail;
    BLE_ADV_REC_PDU rx_buf[REC_PDU_BUFFER_LEN];

    // TX data
    IO* tx_io;
    uint32_t tx_handle;
    BLE_ADV_PKT tx_pdu;
} RADIO_DRV;

void nrf_rf_init(EXO* exo);
void nrf_rf_request(EXO* exo, IPC* ipc);


#endif /* _NRF_RF_H_ */
