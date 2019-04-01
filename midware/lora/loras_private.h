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

#define LORA_DEBUG                                          (LORA_DEVELOPER_MODE)
#define LORA_DEBUG_INFO                                     (1)
#define LORA_DEBUG_IRQS                                     (0)
#define LORA_DEBUG_FIFO_PTRS                                (0)
#define LORA_DEBUG_FIFO_DATA                                (0)
#define LORA_DEBUG_RCVD_PAYLOAD_SIZE                        (0)
#define LORA_DEBUG_STATES                                   (0)

typedef struct {
    IO* io;
    uint32_t port;
} LORA_SPI;

typedef struct {
#if defined (LORA_SX127X) || defined (LORA_SX126X)
    /* write base address in FIFO data buffer for TX modulator */  
    uint8_t     fifo_tx_base_addr; 
    /* read base address in FIFO data buffer for RX demodulator */  
    uint8_t     fifo_rx_base_addr; 
    /* Enable CRC generation and check on payload:
        0 - CRC disable
        1 - CRC enable
        If CRC is needed, RxPayloadCrcOn should be set:
        - in Implicit header mode: on Tx and Rx side
        - in Explicit header mode: on the Tx side alone (recovered from
        the header in Rx side) */
    uint8_t     rx_payload_crc_on;
    /* LowDataRateOptimize:
        0 - Disabled
        1 - Enabled
       SX1272: LDO is mandated for SF11 and SF12 with BW = 125 kHz 
       SX1276: LDO is mandated for when the symbol length exceeds 16ms
       SX1261: LDO is mandated for SF11 and SF12 with BW = 125 kHz
               LDO is mandated for SF12          with BW = 250 kHz */
    uint8_t     low_data_rate_optimize;
    /* SX127X: power amplifier max output power:
        Pout = 2 + OutputPower(3:0) on PA_BOOST.
        Pout = -1 + OutputPower(3:0) on RFIO. */
    /* SX126X: The TX output power is defined as power in dBm in a range of
       -17 (0xEF) to +14 (0x0E) dBm by step of 1 dB if low  power PA is selected
       -9  (0xF7) to +22 (0x16) dBm by step of 1 dB if high power PA is selected */
    int8_t      tx_output_power_dbm;
#endif
#if defined (LORA_SX127X)
    /* Selects PA output pin:
       0 RFIO pin. Output power is limited to 13 dBm.
       1 PA_BOOST pin. Output power is limited to 20 dBm */
    uint8_t     pa_select;
    /* LoRa detection Optimize
        0x03 - SF7 to SF12
        0x05 - SF6 */
    uint8_t     detection_optimize;
    /* LoRa detection threshold
        0x0A - SF7 to SF12
        0x0C - SF6 */
    uint8_t     detection_threshold;
    /* SPI interface address pointer in FIFO data buffer. */
    uint8_t     fifo_addr_ptr; 
    /* 
     * sx1276 related additional settings 
     */
    /* Sets the floor reference for all AGC thresholds:
       AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
       SNR = 8dB, fixed value */
    uint8_t     agc_reference_level;
    /* Defines the 1st AGC Threshold */
    uint8_t     agc_step1;
    /* Defines the 2nd AGC Threshold */
    uint8_t     agc_step2;
    /* Defines the 3rd AGC Threshold */
    uint8_t     agc_step3;
    /* Defines the 4th AGC Threshold */
    uint8_t     agc_step4;
    /* Defines the 5th AGC Threshold */
    uint8_t     agc_step5;
    /* Controls the PLL bandwidth:
        00 - 75 kHz     10 - 225 kHz
        01 - 150 kHz    11 - 300 kHz */
    uint8_t     pll_bandwidth;
    /* Select max output power: Pmax=10.8+0.6*MaxPower [dBm] */
    uint8_t     max_power;
#elif defined (LORA_SX126X)
    /* 9.2.1 Image Calibration for Specific Frequency Bands
       The image calibration is done through the command CalibrateImage(...) 
       for a given range of frequencies defined by the parameters freq1 and 
       freq2. Once performed, the calibration is valid for all frequencies 
       between the two extremes used as parameters. Typically, the user can 
       select the parameters freq1 and freq2 to cover any specific ISM band. */
    uint8_t     freq1;
    uint8_t     freq2;
#endif
} LORA_SYS_CONFIG;

typedef enum {
    LORA_STATUS_TRANSFER_COMPLETED = 0,
    LORA_STATUS_TRANSFER_IN_PROGRESS
} LORA_STATUS;

typedef struct _LORA {
#if defined (LORA_SX127X) || defined (LORA_SX126X)
    HANDLE          process;
    HANDLE          timer_txrx_timeout;
    HANDLE          timer_poll_timeout;
    bool            timer_poll_timeout_stopped;//todo: os timers issue
    LORA_USR_CONFIG    usr_config;
    LORA_SYS_CONFIG    sys_config;
    LORA_SPI        spi;
    LORA_STATUS     status;
    LORA_ERROR      error;
    bool            rxcont_mode_started;
    IO*             io_rx;
    LORA_STATS      stats;
    LORA_REQ        req;
    bool            por_reset_completed;
#if (LORA_DEBUG)
    uint32_t        transfer_in_progress_ms;    //debug
    uint32_t        polls_cnt;                  //debug
#endif
    //internal flags (used for clearer logic)  
    bool            tx_on; 
    bool            rx_on;
#endif
#if defined (LORA_SX126X)
    //cached value to avoid spi transactions (will cause chip wakeup in case of being in sleep)
    uint8_t         chip_state_cached; 
#endif
} LORA;

typedef enum
{
    LORA_TIMER_TXRX_TIMEOUT_ID = 0,
    LORA_TIMER_POLL_TIMEOUT_ID,
} LORA_TIMER_ID;

typedef enum 
{
    LORA_STAT_SEL_NONE = 0,
#if defined (LORA_SX127X) || defined (LORA_SX126X)
    RSSI_CUR_DBM,
    RSSI_LAST_PKT_RCVD_DBM,
    SNR_LAST_PKT_RCVD_DB,
    PKT_TIME_ON_AIR_MS,
    TX_OUTPUT_POWER_DBM,
    RF_FREQ_ERROR_HZ,
#endif  
} LORA_STAT_SEL;

void lora_set_error(LORA* lora, LORA_ERROR error);
#if (LORA_DEBUG)
void lora_set_config_error(LORA* lora, const char* file, const uint32_t line, uint32_t argc, ...);
void lora_set_intern_error(LORA* lora, const char* file, const uint32_t line, uint32_t argc, ...);
#endif

#endif //LORAS_PRIVATE_H
