/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#ifndef LORAS_PRIVATE_H
#define LORAS_PRIVATE_H

#include "sys_config.h"
#include <stdint.h>
#include "object.h"
#include "lora.h"

typedef struct {
    IO*      io;
    uint32_t port;
} LORA_SPI;

typedef enum {
    LORA_STATUS_TRANSFER_COMPLETED = 0,
    LORA_STATUS_TRANSFER_IN_PROGRESS
} LORA_STATUS;

typedef struct _LORA {
    HANDLE           process;
    HANDLE           timer_txrx_timeout;
    HANDLE           timer_poll_timeout;
    LORA_CONFIG      config;
    //private chip specific config variables
    void*            sys_vars;
    LORA_SPI         spi;
    LORA_STATUS      status;
    LORA_STATS_TX    stats_tx;
    LORA_STATS_RX    stats_rx;
    IO*              io;
    bool             tx;
    bool             opened;
    bool             rxcont_mode_started;
    bool             busy_pin_state_invalid;
#if (LORA_DEBUG)
    uint32_t         transfer_in_progress_ms;
    uint32_t         polls_cnt;
#endif
    /* As soon as timeout is expired server polling is stopped */
    uint32_t         tx_timeout_ms;
    uint32_t         rx_timeout_ms;
 } LORA;

typedef enum
{
    LORA_TIMER_TXRX_TIMEOUT_ID = 0,
    LORA_TIMER_POLL_TIMEOUT_ID,
} LORA_TIMER_ID;

typedef enum
{
    PKT_TIME_ON_AIR_MS = 0,
    PKT_TIME_ON_AIR_MAX_MS,
    TX_OUTPUT_POWER_DBM,
    RX_RSSI_CUR_DBM,
    RX_RSSI_LAST_PKT_RCVD_DBM,
    RX_SNR_LAST_PKT_RCVD_DB,
    RX_RF_FREQ_ERROR_HZ,
} LORA_STAT;

bool loras_hw_open         (LORA* lora);
void loras_hw_close        (LORA* lora);
void loras_hw_sleep        (LORA* lora);
void loras_hw_tx_async     (LORA* lora, IO* io);
void loras_hw_rx_async     (LORA* lora, IO* io);
void loras_hw_tx_async_wait(LORA* lora);
void loras_hw_rx_async_wait(LORA* lora);
uint32_t loras_hw_get_stat (LORA* lora, LORA_STAT stat);

#endif //LORAS_PRIVATE_H
