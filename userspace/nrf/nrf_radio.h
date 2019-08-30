/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_RADIO_H_
#define _NRF_RADIO_H_

#include "nrf_driver.h"
#include "nrf_radio_config.h"
#include "../ipc.h"
#include "../io.h"

#define RADIO_FLAG_EMPTY                                                (0 << 0)
#define RADIO_FLAG_TIMEOUT                                              (1 << 0)

typedef enum {
    RADIO_SET_CHANNEL= IPC_USER,
    RADIO_SET_TXPOWER,
    RADIO_SET_ADDRESS,
    RADIO_START,
    RADIO_STOP,
    // BLE
//    BLE_ADVERTISE_LISTEN,
//    BLE_SEND_ADV_DATA
} RADIO_IPCS;

typedef struct {
    unsigned int flags;
    unsigned int timeout_ms;
} RADIO_STACK;

HANDLE radio_open(char* process_name, RADIO_MODE mode);
void radio_close();

void radio_start(IO* io);
//void radio_stop();
//void radio_tx();
//void radio_rx();
bool radio_tx_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int timeout_ms);
bool radio_rx_size_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int size, unsigned int timeout_ms);
int radio_rx_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int timeout_ms);
void radio_set_channel(uint8_t channel);
void radio_set_tx_power(uint8_t tx_power_dBm);


#endif /* _NRF_RADIO_H_ */
