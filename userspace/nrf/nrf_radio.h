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

typedef enum {
    RADIO_SET_CHANNEL= IPC_USER,
    RADIO_START,
    RADIO_STOP,
    RADIO_TX,
    RADIO_RX,
    // BLE
    BLE_ADVERTISE_LISTEN,
    BLE_SEND_ADV_DATA
} RADIO_IPCS;

typedef struct {
    uint8_t flags;
    unsigned int timeout;
}   RADIO_STACK;

HANDLE radio_open(char* process_name, RADIO_MODE mode);
void radio_close();

//void radio_start();
//void radio_stop();
//void radio_tx();
//void radio_rx();
//void radio_tx_sync();
//void radio_rx_sync();
//void radio_set_channel(uint8_t channel);


#endif /* _NRF_RADIO_H_ */
