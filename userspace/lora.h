/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include "object.h"
#include "ipc.h"
#include "io.h"

#include "sx12xx_config.h"

typedef enum {
    LORA_TRANSFER_COMPLETED = IPC_USER,
    LORA_TRANSFER_IN_PROGRESS,
    LORA_GET_STATS,
    LORA_CLEAR_STATS,
} LORA_IPCS;

typedef struct {
    uint8_t     spi_port;
    uint16_t    spi_cs_pin;
    uint16_t    reset_pin;
#if (LORA_CHIP == SX1276)
    /* This pin must be set/reset based on TX/RX operation */
    uint16_t    rxtx_ext_pin;
#elif (LORA_CHIP == SX1261)
    /* The BUSY control line is used to indicate the status of the internal
       state machine. When the BUSY line is held low, it indicates that the
       internal state machine is in idle mode and that the radio is ready to
       accept a command from the host controller. */
    uint16_t    busy_pin;
#endif
} LORA_CONFIG;

typedef struct {
    /* Total packet time on air */
    uint32_t    pkt_time_on_air_ms;
    /* Number of cases when no TX_DONE irq is generated => timeout expired */
    uint32_t    timeout_num;
    /* Power amplifier max output power */
    int8_t      output_power_dbm;
} LORA_STATS_TX;

typedef struct {
    /* Total packet time on air */
    uint32_t    pkt_time_on_air_ms;
    /* Number of cases when no preamble received => timeout expired */
    uint32_t    timeout_num;
    /* Number of CRC errors */
    uint32_t    crc_err_num;
    /* Current RSSI value (dBm) */
    int16_t     rssi_cur_dbm;
    /* RSSI of the latest packet received (dBm) */
    int16_t     rssi_last_pkt_rcvd_dbm;
    /* Estimation of SNR on last packet received (dB) */
    int8_t      snr_last_pkt_rcvd_db;
    /* Estimated frequency error from modem */
    int32_t     rf_freq_error_hz;
} LORA_STATS_RX;

HANDLE lora_create(uint32_t process_size, uint32_t priority);

int  lora_open(HANDLE lora, IO* io_config);
void lora_close(HANDLE lora);

void lora_tx_async(HANDLE lora, IO* io);
void lora_rx_async(HANDLE lora, IO* io);

void lora_get_stats(HANDLE lora, IO* io_stats_tx, IO* io_stats_rx);
void lora_clear_stats(HANDLE lora);

void lora_abort_rx_transfer(HANDLE lora);

#endif // LORA_H
