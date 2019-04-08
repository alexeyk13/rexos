/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_RADIO_H_
#define _NRF_RADIO_H_

#include "nrf_driver.h"

typedef enum {
    RADIO_ADVERTISE_LISTEN = IPC_USER,
    RADIO_START,
    RADIO_STOP,
    RADIO_TX,
    RADIO_RX,
} ADC_IPCS;

typedef struct {
    uint8_t flags;
    unsigned int timeout;
}   RADIO_STACK;

void radio_open(RADIO_MODE mode);
void radio_close();

void radio_start();
void radio_stop();

void radio_tx();
void radio_rx();
void radio_tx_sync();
void radio_rx_sync();

bool radio_get_advertise(unsigned int max_size, uint8_t flags, unsigned int timeout_ms);


#endif /* _NRF_RADIO_H_ */
