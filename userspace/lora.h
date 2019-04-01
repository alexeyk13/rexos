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


typedef enum {
    LORA_IDLE = IPC_USER,
    LORA_TRANSFER_COMPLETED,
    LORA_TRANSFER_IN_PROGRESS,
    LORA_GET_STATS,
    LORA_CLEAR_STATS,
    LORA_ABORT_RX_TRANSFER
} LORA_IPCS;

typedef enum {
    LORA_ERROR_NONE = 0,
    LORA_ERROR_RX_TRANSFER_ABORTED,
#if defined (LORA_SX127X) || defined (LORA_SX126X)
    LORA_ERROR_TX_TIMEOUT,
    LORA_ERROR_RX_TIMEOUT,
    LORA_ERROR_PAYLOAD_CRC_INCORRECT,
    LORA_ERROR_CANNOT_READ_FIFO,
    LORA_ERROR_INTERNAL,
    LORA_ERROR_CONFIG,
#endif
#if defined (LORA_SX127X)
    LORA_ERROR_NO_CRC_ON_THE_PAYLOAD,
#endif
} LORA_ERROR;

static inline char* lora_error_str(LORA_ERROR error)
{
    switch(error)
    {
    case LORA_ERROR_NONE:                   return "LORA_ERROR_NONE";
    case LORA_ERROR_RX_TRANSFER_ABORTED:    return "LORA_ERROR_RX_TRANSFER_ABORTED";
#if defined (LORA_SX127X) || defined (LORA_SX126X)
    case LORA_ERROR_TX_TIMEOUT:             return "LORA_ERROR_TX_TIMEOUT";
    case LORA_ERROR_RX_TIMEOUT:             return "LORA_ERROR_RX_TIMEOUT";
    case LORA_ERROR_PAYLOAD_CRC_INCORRECT:  return "LORA_ERROR_PAYLOAD_CRC_INCORRECT";
    case LORA_ERROR_CANNOT_READ_FIFO:       return "LORA_ERROR_CANNOT_READ_FIFO";
    case LORA_ERROR_INTERNAL:               return "LORA_ERROR_INTERNAL";
    case LORA_ERROR_CONFIG:                 return "LORA_ERROR_CONFIG";
#endif
#if defined (LORA_SX127X)
    case LORA_ERROR_NO_CRC_ON_THE_PAYLOAD:  return "LORA_ERROR_NO_CRC_ON_THE_PAYLOAD";
#endif
    default: return "LORA_ERROR_UNKNOWN";
    }
}

/* Semtech ID relating the silicon revision */
typedef enum {
    LORA_CHIP_UNKNOWN = 0,
#if defined (LORA_SX127X)
    LORA_CHIP_SX1272,
    LORA_CHIP_SX1276,
#elif defined (LORA_SX126X)
    LORA_CHIP_SX1261,
#endif
    LORA_CHIP_MAX
} LORA_CHIP;

typedef enum {
    LORA_HEADER_ON = 0,
    LORA_HEADER_OFF,
} LORA_OPT_HEADER;

typedef enum {
    LORA_CRC_ON = 0,
    LORA_CRC_OFF,
} LORA_OPT_CRC;

typedef enum {
    /* In this mode, the modem searches for a preamble during a given period of time.
        If a preamble hasn’t been found at the end of the time window, the chip generates
        the RxTimeout interrupt and goes back to Standby mode. */
    LORA_SINGLE_RECEPTION = 0,
    /* In continuous receive mode, the modem scans the channel continuously for a preamble.
        Each time a preamble is detected the modem tracks it until the packet is received and
        then carries on waiting for the next preamble. */
    LORA_CONTINUOUS_RECEPTION
} LORA_RECV_MODE;

typedef enum {
    /* The explicit packet includes a short header that contains information about the number
        of bytes, coding rate and whether a CRC is used in the packet. The packet format is
        shown in the following figure. */
    LORA_EXPLICIT_HEADER_MODE = 0,
    /* In certain scenarios, where the payload, coding rate and CRC presence are fixed or known
        in advance, it may be advantageous to reduce transmission time by invoking implicit
        header mode. In this mode the header is removed from the packet. In this case the payload
        length, error coding rate and presence of the payload CRC must be manually configured on
        both sides of the radio link. */
    LORA_IMPLICIT_HEADER_MODE
} LORA_HEADER_MODE;

/* This LoRa instance is TX or RX or both TX-RX */
typedef enum {
    LORA_TX = 0,
    LORA_RX,
    LORA_TXRX,      //NOTE: LORA_TXRX is possible to run only on sx1272
} LORA_TXRX_MODE;

typedef enum {
    LORA_REQ_NONE = 0,
    LORA_REQ_SEND,
    LORA_REQ_RECV,
} LORA_REQ;

typedef struct {
    uint8_t     spi_port;
    uint32_t    spi_baudrate_mbps;   
    uint16_t    spi_cs_pin;         uint16_t    spi_cs_pin_mode;    
    uint16_t    spi_sck_pin;        uint16_t    spi_sck_pin_mode;    
    uint16_t    spi_sout_pin;       uint16_t    spi_sout_pin_mode;    
    uint16_t    spi_sin_pin;        uint16_t    spi_sin_pin_mode;    
    uint16_t    reset_pin;          uint16_t    reset_pin_mode;   

    LORA_CHIP           chip;
    LORA_TXRX_MODE      txrx;

#if defined (LORA_SX127X) || defined (LORA_SX126X)
    LORA_RECV_MODE      recv_mode;
    /* ImplicitHeaderModeOn:
        0 - Explicit Header mode
        1 - Implicit Header mode */
    uint8_t     implicit_header_mode_on;
    /* RF carrier frequency */
    uint32_t    rf_carrier_frequency_mhz;
    /* SF rate (expressed as a base-2 logarithm)
         6 -   64 chips / symbol
         7 -  128 chips / symbol
         8 -  256 chips / symbol
         9 -  512 chips / symbol
        10 - 1024 chips / symbol
        11 - 2048 chips / symbol
        12 - 4096 chips / symbol
        other values reserved. */    
    uint8_t     spreading_factor;
       /* As soon as timeout is expired server polling is stopped */
    uint32_t    tx_timeout_ms;
    uint32_t    rx_timeout_ms;
    /* Payload length in bytes. The register needs to be set in implicit
        header mode for the expected packet length. A 0 value is not
        permitted */
    uint8_t     payload_length;
    /* Maximum payload length; if header payload length exceeds
        value a header CRC error is generated. Allows filtering of packet
        with a bad size. */
    uint8_t     max_payload_length;
    /* Signal bandwidth (SX1272):
        00 - 125 kHz
        01 - 250 kHz
        10 - 500 kHz
        11 - reserved */ 
    /* Signal bandwidth (SX1276):
        0000 - 7.8   kHz      0001 - 10.4 kHz
        0010 - 15.6  kHz      0011 - 20.8 kHz
        0100 - 31.25 kHz      0101 - 41.7 kHz
        0110 - 62.5  kHz      0111 - 125  kHz
        1000 - 250   kHz      1001 - 500  kHz
        other values - reserved
        In the lower band (169MHz), signal bandwidths 8&9 are not supported) */
    /* Signal bandwidth (SX1261):
       0x00 - 7.81  kHz       0x08 - 10.42 kHz
       0x01 - 15.63 kHz       0x09 - 20.83 kHz
       0x02 - 31.25 kHz       0x0A - 41.67 kHz
       0x03 - 62.50 kHz       0x04 - 125   kHz
       0x05 - 250   kHz       0x06 - 500   kHz */
    uint8_t     signal_bandwidth;
    /* SX127X: Enables the +20 dBm option on PA_BOOST pin */
    /* SX126X: Selection between high power PA and low power PA */
    bool        enable_high_power_pa;
#endif
#if defined (LORA_SX127X)
    /* SF = 6 Is a special use case for the highest data rate. */
    bool        enable_SF_6;
#elif defined (LORA_SX126X)
    /* The BUSY control line is used to indicate the status of the internal 
       state machine. When the BUSY line is held low, it indicates that the 
       internal state machine is in idle mode and that the radio is ready to 
       accept a command from the host controller. */
    uint16_t    busy_pin;    
    uint16_t    busy_pin_mode; 
    /* DIO2 can be configured to drive an RF switch through the use 
       of the command SetDio2AsRfSwitchCtrl(...). In this mode, DIO2
       will be at a logical 1 during Tx and at a logical 0 in any 
       other mode */
    bool        dio2_as_rf_switch_ctrl;
    /* DIO3 can be used to automatically control a TCXO through the command 
       SetDio3AsTCXOCtrl(...). In this case, the device will automatically power 
       cycle the TCXO when needed. */
    bool        dio3_as_tcxo_ctrl;
     /* By default, all IRQ are masked (all ‘0’) and the user 
       can enable them one by one (or several at a time) by 
       setting the corresponding mask to ‘1’.*/
    uint32_t    dio1_mask;
    uint32_t    dio2_mask;
    uint32_t    dio3_mask;   
#endif
} LORA_USR_CONFIG;

typedef struct {
#if defined (LORA_SX127X) || defined (LORA_SX126X)
    /* Number of cases when no TX_DONE irq is generated => tx_timeout expired */
    uint32_t    tx_timeout_num;
    /* Number of cases when no preamble received => rx_timeout expired */
    uint32_t    rx_timeout_num;
    /* Number of CRC errors */
    uint32_t    crc_err_num;
    /* Power amplifier max output power */
    int8_t      tx_output_power_dbm;
    /* Current RSSI value (dBm) */
    int16_t     rssi_cur_dbm;
    /* RSSI of the latest packet received (dBm) */
    int16_t     rssi_last_pkt_rcvd_dbm;
    /* Estimation of SNR on last packet received (dB) */
    int8_t      snr_last_pkt_rcvd_db;
    /* Total packet time on air */
    uint32_t    pkt_time_on_air_ms;
    /* Estimated frequency error from modem */
    int32_t     rf_freq_error_hz;
#endif  
} LORA_STATS;

HANDLE lora_create(uint32_t process_size, uint32_t priority);

void lora_open(HANDLE lora, IO* io_config);
void lora_close(HANDLE lora);

void lora_tx_async(HANDLE lora, IO* io);
void lora_rx_async(HANDLE lora, IO* io);

void lora_get_stats(HANDLE lora, LORA_STATS* stats);
void lora_clear_stats(HANDLE lora);
void lora_abort_rx_transfer(HANDLE lora);

#endif // LORA_H
