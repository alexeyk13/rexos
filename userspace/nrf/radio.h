/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_RADIO_H_
#define _NRF_RADIO_H_

#include "nrf_driver.h"
#include "radio_config.h"

typedef enum {
    RADIO_ADVERTISE_LISTEN = IPC_USER,
    RADIO_SET_CHANNEL,
    RADIO_SEND_ADV_DATA,
    RADIO_START,
    RADIO_STOP,
    RADIO_TX,
    RADIO_RX,
} RADIO_IPCS;

typedef struct {
    uint8_t flags;
    unsigned int timeout;
}   RADIO_STACK;

HANDLE radio_open(char* process_name, RADIO_MODE mode);
HANDLE ble_open();

void radio_close();

void radio_start();
void radio_stop();

void radio_tx();
void radio_rx();
void radio_tx_sync();
void radio_rx_sync();

void radio_set_channel(uint8_t channel);

// send advertise packet
bool radio_send_adv(uint8_t channel, uint8_t* adv_data, unsigned int data_size);

//bool radio_listen_adv_channel(uint8_t channel, uint8_t flags, unsigned int timeout_ms);
bool radio_listen_adv_channel(unsigned int max_size, uint8_t flags, unsigned int timeout_ms);


#endif /* _NRF_RADIO_H_ */
