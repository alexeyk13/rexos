/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_BLE_H_
#define _NRF_BLE_H_

#include "nrf_radio.h"
#include "nrf_radio_config.h"
#include "nrf_ble_config.h"
#include "../io.h"

HANDLE ble_open();
void ble_close();

// send advertise packet
bool ble_send_adv(uint8_t channel, uint8_t* adv_data, unsigned int data_size);
bool ble_listen_adv_channel(unsigned int max_size, uint8_t flags, unsigned int timeout_ms);


/* debug */
void ble_debug_adv_common(IO* io);


#endif /* _NRF_BLE_H_ */
