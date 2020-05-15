/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_radio.h"
#include "sys_config.h"
#include "object.h"
#include "stdio.h"
#include <string.h>

bool radio_open(void* param)
{
    return get_handle_exo(HAL_REQ(HAL_RF, IPC_OPEN), 0, 0, (uint32_t)param) != INVALID_HANDLE;
}


int radio_tx_sync(IO* io, uint8_t channel, uint32_t id, uint32_t flags)
{
    return io_read_sync_exo(HAL_IO_REQ(HAL_RF, IPC_WRITE), channel | (flags <<16), io, id);
}

void radio_rx(IO* io)
{
    io_read_exo(HAL_IO_REQ(HAL_RF, IPC_READ), 0, io, 0);
}

void radio_cancel_rx()
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, IPC_CANCEL_IO), 0, 0, 0);
}

void radio_set_mac_addr(const BLE_MAC* addr)
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_SET_ADV_ADDRESS), 0, addr->hi, addr->lo);
}


void radio_set_channel(uint8_t channel)
{
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_SET_CHANNEL), channel, 0, 0);
}

void radio_set_tx_power(int8_t tx_power_dBm)
{
    if(tx_power_dBm < RADIO_MIN_POWER_DBM)
        tx_power_dBm = RADIO_MIN_POWER_DBM;
    if(tx_power_dBm > RADIO_MAX_POWER_DBM)
        tx_power_dBm = RADIO_MAX_POWER_DBM;
    ack(KERNEL_HANDLE, HAL_REQ(HAL_RF, RADIO_SET_TXPOWER), tx_power_dBm, 0, 0);
}

int ble_adv_add_flags(IO* io, uint8_t flags)
{
    return ble_adv_add_data(io, BLE_ADV_TYPE_FLAGS, &flags, 1);
}

int ble_adv_add_data(IO* io, uint8_t type, const uint8_t* src, uint32_t data_size)
{
    uint32_t pos = io->data_size;
    uint8_t* ptr = io_data(io) + pos;
    int rest = BLE_ADV_DATA_SIZE - 2 - pos - data_size;
    if(rest < 0)
        return rest;
    io->data_size += data_size + 2;
    *ptr++ = (data_size + 1) & 0xff;
    *ptr++ = type;
    while(data_size--)
        *ptr++ = *src++;
    return rest;
}

int ble_adv_add_str(IO* io, uint8_t type, const char* src)
{
    return ble_adv_add_data(io, type, (uint8_t*)src, strlen(src));
}


