/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#include "loras_private.h"
#include "../userspace/gpio.h"
#include "pin.h"
#include "spi.h"

#if (LORA_DEBUG)
#include "stdio.h"
#endif

#include <stdlib.h>
#include <string.h>

//-------------------- local settings depending on LORA_xx settings ------------------
//Image Calibration for Specific Frequency Bands (9.2.1)
//The image calibration is done through the command CalibrateImage(...)
//for a given range of frequencies defined by the parameters freq1 and
//freq2. Once performed, the calibration is valid for all frequencies
//between the two extremes used as parameters. Typically, the user can
//select the parameters freq1 and freq2 to cover any specific ISM band.
//  Frequency Band [MHz]    Freq1           Freq2
//  430 - 440               0x6B            0x6F
//  470 - 510               0x75            0x81
//  779 - 787               0xC1            0xC5
//  863 - 870               0xD7            0xDB
//  902 - 928               0xE1 (default)  0xE9 (default)
#define FREQ LORA_RF_CARRIER_FREQUENCY_MHZ
#if   (FREQ >= 430 && FREQ <= 440)
#define FREQ1                                               0x6B
#define FREQ2                                               0x6F
#elif (FREQ >= 470 && FREQ <= 510)
#define FREQ1                                               0x75
#define FREQ2                                               0x81
#elif (FREQ >= 779 && FREQ <= 787)
#define FREQ1                                               0xC1
#define FREQ2                                               0xC5
#elif (FREQ >= 863 && FREQ <= 870)
#define FREQ1                                               0xD7
#define FREQ2                                               0xDB
#elif (FREQ >= 902 && FREQ <= 928)
#define FREQ1                                               0xE1
#define FREQ2                                               0xE9
#else
#error unsupported frequency
#endif
#undef FREQ
//The TX output power is defined as power in dBm in a range of
//  -17 (0xEF) to +14 (0x0E) dBm by step of 1 dB if low  power PA is selected
//  -9  (0xF7) to +22 (0x16) dBm by step of 1 dB if high power PA is selected */
//NOTE: The default maximum RF output power of the transmitter is
//      +14/15 dBm for SX1261 and +22 dBm for SX1262.
//NOTE: By default low power PA and +14 dBm are set
#if (LORA_PA_SELECT==1)
#define OUTPUT_POWER_DBM                                    15
#else
#define OUTPUT_POWER_DBM                                    14
#endif

//LowDataRateOptimize: 0 = Disabled     1 = Enabled
//SX1261: LDO is mandated for SF11 and SF12 with BW = 125 kHz
//        LDO is mandated for SF12          with BW = 250 kHz */
#if ((LORA_SPREADING_FACTOR == 11 && LORA_SIGNAL_BANDWIDTH == 0x04) || \
     (LORA_SPREADING_FACTOR == 12 && LORA_SIGNAL_BANDWIDTH == 0x04) || \
     (LORA_SPREADING_FACTOR == 12 && LORA_SIGNAL_BANDWIDTH == 0x05))
#define LOW_DATA_RATE_OPTIMIZE                              1
#else
#define LOW_DATA_RATE_OPTIMIZE                              0
#endif

//check config correctness
#include "check_config.h"
//-------------------- local settings depending on LORA_xx settings (end) ------------

//Write base address in FIFO data buffer for TX modulator
#define FIFO_TX_BASE_ADDR                                   0x00
//Read  base address in FIFO data buffer for RX demodulator
#define FIFO_RX_BASE_ADDR                                   0x00
//after each command is issued get its status and compare with CS_RFU
#define CHECK_CMD_STATUS                                    0
//after each byte is transferred via spi get its status and compare with CS_RFU
#define CHECK_CMD_BYTE_STATUS                               0

/*
 * Table 12-1: List of Registers
 */
//Differentiate the LoRa® signal for Public or Private Network
#define REG_LORA_SYNC_WORD_MSB                              0x0740
#define REG_LORA_SYNC_WORD_MSB_RESET                        0x14
//Set to 0x3444 for Public Network Set to 0x1424 for Private Network
#define REG_LORA_SYNC_WORD_LSB                              0x0741
#define REG_LORA_SYNC_WORD_LSB_RESET                        0x24
//Can be used to get a 32-bit random number
#define REG_RANDOM_NUMBER_GEN_0                             0x0819
#define REG_RANDOM_NUMBER_GEN_1                             0x081A
#define REG_RANDOM_NUMBER_GEN_2                             0x081B
#define REG_RANDOM_NUMBER_GEN_3                             0x081C
//Set the gain used in Rx mode: Rx Power Saving gain: 0x94 Rx Boosted gain: 0x96
#define REG_RX_GAIN                                         0x08AC
#define REG_RX_GAIN_RESET                                   0x94
//Set the Over Current Protection level. The value is changed internally depending
//on the device selected. Default values are: SX1262: 0x38 (140 mA) SX1261: 0x18 (60 mA)
#define REG_OCP_CONFIGURATION                               0x08E7
#define REG_OCP_CONFIGURATION_RESET                         0x18
//Value of the trimming cap on XTA pin This register should only be changed while
//the radio is in OM_STDBY_XOSC mode.
#define REG_XTA_TRIM                                        0x0911
#define REG_XTA_TRIM_RESET                                  0x05
//Value of the trimming cap on XTB pin This register should only be changed while
//the radio is in OM_STDBY_XOSC mode.
#define REG_XTB_TRIM                                        0x0912
#define REG_XTB_TRIM_RESET                                  0x05
//The address of the register holding frequency error indication
//NOTE: not documented??
#define REG_FREQ_ERROR                                      0x076B
//The address of the register holding the payload size
//NOTE: not documented??
#define REG_PAYLOAD_LENGTH                                  0x0702
//Timeout limits
#define TIMEOUT_MAX_US                                      (262*1000*1000)
//round-up 15.625 us
#define TIMEOUT_MIN_US                                      16
#define TIMEOUT_MAX_VAL                                     0xFFFFFF

//commands (sorted by their occurrence in the code below)
#define CMD_GET_STATUS                                      0xC0
#define CMD_GET_DEVICE_ERRORS                               0x17
#define CMD_CLEAR_DEVICE_ERRORS                             0x07
#define CMD_SET_SLEEP                                       0x84
#define CMD_SET_STANDBY                                     0x80
#define CMD_SET_FS                                          0xC1
#define CMD_SET_TX                                          0x83
#define CMD_SET_RX                                          0x82
#define CMD_STOP_TIMER_ON_PREAMBLE                          0x9F
#define CMD_SET_RX_DUTY_CYCLE                               0x94
#define CMD_SET_CAD                                         0xC5
#define CMD_SET_TX_CONTINUOUS_WAVE                          0xD1
#define CMD_SET_TX_INFINITE_PREAMBLE                        0xD2
#define CMD_SET_REGULATOR_MODE                              0x96
#define CMD_CALIBRATE                                       0x89
#define CMD_CALIBRATE_IMAGE                                 0x98
#define CMD_SET_PA_CONFIG                                   0x95
#define CMD_SET_RX_TX_FALLBACK_MODE                         0x93
#define CMD_WRITE_REGISTER                                  0x0D
#define CMD_READ_REGISTER                                   0x1D
#define CMD_WRITE_BUFFER                                    0x0E
#define CMD_READ_BUFFER                                     0x1E
#define CMD_SET_DIO_IRQ_PARAMS                              0x08
#define CMD_GET_IRQ_STATUS                                  0x12
#define CMD_CLEAR_IRQ_STATUS                                0x02
#define CMD_SET_DIO2_AS_RF_SWITCH_CTRL                      0x9D
#define CMD_SET_DIO3_AS_TCXO_CTRL                           0x97
#define CMD_SET_RF_FREQUENCY                                0x86
#define CMD_SET_PACKET_TYPE                                 0x8A
#define CMD_GET_PACKET_TYPE                                 0x11
#define CMD_SET_TX_PARAMS                                   0x8E
#define CMD_SET_MODULATION_PARAMS                           0x8B
#define CMD_SET_PACKET_PARAMS                               0x8C
#define CMD_SET_CAD_PARAMS                                  0x88
#define CMD_SET_BUFFER_BASE_ADDRESS                         0x8F
#define CMD_SET_LORA_SYMB_NUM_TIMEOUT                       0xA0
#define CMD_GET_RSSI_INST                                   0x15
#define CMD_GET_RX_BUFFER_STATUS                            0x13
#define CMD_GET_PACKET_STATUS                               0x14
#define CMD_GET_STATS                                       0x10
#define CMD_RESET_STATS                                     0x00

//precision error increase macro (with magic numbers)
#define PREC_ERROR_01(arg)                                  (0 - (arg / 48830))
#define PREC_ERROR_02(arg)                                  (1 - (arg / 9750))
#define PREC_ERROR_03(arg)                                  (arg >= 678428673 ? -1 : 0)

//to keep precision with integer arith.
#define DOMAIN_US                                           1000000

typedef struct {
    //cached value to avoid spi transactions (will cause chip
    //wakeup in case of being in sleep)
    uint8_t     chip_state_cached;
    //add more (if need)
} SYS_VARS;

typedef enum
{
    ST_RESET = 0,
    ST_STARTUP,
    ST_SLEEP_COLD_START,
    ST_SLEEP_WARM_START,
    ST_STDBY_RC,
    ST_STDBY_XOSC,
    ST_FS,
    ST_TX,
    ST_RX,
    ST_UNKNOWN = 0xFF
} state_t;

/* Table 13-77: Status Bytes Definition */
typedef enum
{
    CS_RESERVED = 0,
    CS_RFU,
    CS_DATA_IS_AVAILABLE_TO_HOST,
    CS_COMMAND_TIMEOUT,
    CS_COMMAND_PROCESSING_ERROR,
    CS_FAILURE_TO_EXECUTE_COMMAND,
    CS_COMMAND_TX_DONE,
    CS_UNKNOWN = 0xFF
} cmd_status_t;

/* Table 13-77: Status Bytes Definition */
typedef enum
{
    CM_UNUSED = 0,
    CM_RFU,
    CM_STBY_RC,
    CM_STBY_XOSC,
    CM_FS,
    CM_RX,
    CM_TX,
} chip_mode_t;

/* Table 13-85: OpError Bits */
typedef enum
{
    OE_RC64K_CALIB_ERR = 0,
    OE_RC13M_CALIB_ERR,
    OE_PLL_CALIB_ERR,
    OE_ADC_CALIB_ERR,
    OE_IMG_CALIB_ERR,
    OE_XOSC_START_ERR,
    OE_PLL_LOCK_ERR,
    OE_RFU,
    OE_PA_RAMP_ERR,
    OE_RFU_15_9
} op_error_t;

/* 11.1 Operational Modes Commands */
/*
 * Table 11-1: Commands Selecting the Operating Modes of the Radio
 *             Command Opcode Parameters Description
 */
/* Table 13-2: Sleep Mode Definition */
typedef struct
{
    // 0: RTC timeout disable
    // 1: wake-up on RTC timeout (RC64k)
    uint8_t SleepConfig_0   : 1;
    // 0: RFU
    uint8_t SleepConfig_1   : 1;
    // 0: cold start,
    // 1: warm start (device configuration in retention)
    uint8_t SleepConfig_2   : 1;
    // RESERVED
    uint8_t SleepConfig_7_3 : 5;
} sleep_config_t;

/* Table 13-4: STDBY Mode Configuration */
typedef enum
{
    /* Device running on RC13M, set OM_STDBY_RC mode
       NOTE 0: RC13M - 13 MHz Resistance-Capacitance Oscillator
       NOTE 1: The 13 MHz RC oscillator (RC13M) is enabled for all SPI communication
       to permit configuration of the device without the need to start the crystal
       oscillator. */
    CONF_STDBY_RC   = 0,
    /* Device running on XTAL 32MHz, set OM_STDBY_XOSC mode */
    CONF_STDBY_XOSC = 1
} standby_config_t;

/* Table 13-11: StopOnPreambParam Definition */
typedef enum
{
    /* Timer is stopped upon Sync Word or Header detection */
    STOP_ON_PREAMBLE_DISABLE = 0,
    /* Timer is stopped upon preamble detection */
    STOP_ON_PREAMBLE_ENABLE = 1,
} stop_on_preamble_param_t;

typedef struct
{
    /* Table 13-18: Calibration Setting */
    /* 0: RC64k calibration disabled
       1: RC64k calibration enabled */
    uint8_t bit_0_rc64k_calibration_enabled : 1;
    /* 0: RC13Mcalibration disabled
       1: RC13M calibration enabled */
    uint8_t bit_1_rc13m_calibration_enabled : 1;
    /* 0: PLL calibration disabled
       1: PLL calibration enabled */
    uint8_t bit_2_pll_calibration_enabled : 1;
    /* 0: ADC pulse calibration disabled
       1: ADC pulse calibration enabled */
    uint8_t bit_3_adc_pulse_calibration_enabled : 1;
    /* 0: ADC bulk N calibration disabled
       1: ADC bulk N calibration enabled */
    uint8_t bit_4_adc_bulk_n_calibration_enabled : 1;
    /* 0: ADC bulk P calibration disabled
       1: ADC bulk P calibration enabled */
    uint8_t bit_5_adc_bulk_p_calibration_enabled : 1;
    /* 0: Image calibration disabled
       1: Image calibration enabled */
    uint8_t bit_6_image_calibration_enabled : 1;
    /* 0: RFU */
    uint8_t bit_7_rfu : 1;
} calib_param_t;

typedef struct
{
    //tbd
} fallback_mode_t;

/* Table 8-4: IRQ Status Registers
Bit IRQ                         Description                         Protocol
0   TxDone                      Packet transmission completed       All
1   RxDone                      Packet received                     All
2   PreambleDetected            Preamble detected                   All
3   SyncWordValid               Valid Sync Word detected            FSK
4   HeaderValid                 Valid LoRa Header received          LoRa®
5   HeaderErr                   LoRa® header CRC error              LoRa®
6   CrcErr                      Wrong CRC received                  All
7   CadDone                     Channel activity detection finished LoRa®
8   CadDetected                 Channel activity detected           LoRa®
9   Timeout                     Rx or Tx Timeout                    All */
typedef enum
{
    IRQ_TX_DONE = 0,
    IRQ_RX_DONE,
    IRQ_PREAMBLE_DETECTED,
    IRQ_SYNC_WORD_VALID,
    IRQ_HEADER_VALID,
    IRQ_HEADER_ERR,
    IRQ_CRC_ERR,
    IRQ_CAD_DONE,
    IRQ_CAD_DETECTED,
    IRQ_TIMEOUT,
} irq_t;

/* Table 13-38: PacketType Definition */
typedef enum
{
    /* GFSK packet type */
    PACKET_TYPE_GFSK = 0,
    /* LORA mode */
    PACKET_TYPE_LORA = 1
} packet_type_t;

typedef struct
{
    //tbd
} stats_t;

/* Table 13-48: LoRa® ModParam2 - BW */
/* 0x00 LORA_BW_7   (7.81  kHz real)
   0x08 LORA_BW_10  (10.42 kHz real)
   0x01 LORA_BW_15  (15.63 kHz real)
   0x09 LORA_BW_20  (20.83 kHz real)
   0x02 LORA_BW_31  (31.25 kHz real)
   0x0A LORA_BW_41  (41.67 kHz real)
   0x03 LORA_BW_62  (62.50 kHz real)
   0x04 LORA_BW_125 (125   kHz real)
   0x05 LORA_BW_250 (250   kHz real)
   0x06 LORA_BW_500 (500   kHz real) */
typedef enum
{
    LORA_BW_500  = 6,
    LORA_BW_250  = 5,
    LORA_BW_125  = 4,
    LORA_BW_062  = 3,
    LORA_BW_041  = 10,
    LORA_BW_031  = 2,
    LORA_BW_020  = 9,
    LORA_BW_015  = 1,
    LORA_BW_010  = 8,
    LORA_BW_007  = 0,
} bandwidth_t;

typedef enum
{
    RM_LDO  = 0x00, // default
    RM_DCDC = 0x01,
} regulator_mode_t;

// Several forward declarations needed
static inline void set_state(LORA* lora, state_t state_new, ...);

static inline void wait_busy_pin_failed(LORA* lora)
{
    lora->busy_pin_state_invalid = true;
}
static inline void wait_busy_pin_common(LORA* lora, uint8_t value)
{
    uint32_t i;
    LORA_CONFIG* config = &lora->config;
    for (i = 0; i < SX126X_WAIT_BUSY_TIMEOUT_US; ++i)
    {
        if (gpio_get_pin(config->busy_pin) == value)
        {
            return;
        }
        sleep_us(1);
    }
    wait_busy_pin_failed(lora);
}
static inline void wait_busy_pin_high(LORA* lora)
{
    wait_busy_pin_common(lora, 1);
}
static inline void wait_busy_pin_low(LORA* lora)
{
    wait_busy_pin_common(lora, 0);
}
static inline bool wait_busy_pin(LORA* lora)
{
    SYS_VARS* sys_vars = lora->sys_vars;

    //If busy pin is in invalid state (ex. result of prev. check) then skip all the remaining
    //commands of the current lora IPC call (i.e. open/close/tx_async/rx_async/etc.).
    //NOTE: busy_pin_state_invalid will be checked at the end of IPC call.
    if (lora->busy_pin_state_invalid)
        return false;

    if (sys_vars->chip_state_cached == ST_SLEEP_COLD_START ||
        sys_vars->chip_state_cached == ST_SLEEP_WARM_START)
    {
        /* In Sleep mode, the BUSY pin is held high through a 20 kΩ resistor
           and the BUSY line will go low as soon as the radio leaves the Sleep
           mode. */
        wait_busy_pin_high(lora);
    }
    else
    {
        /* The BUSY control line is used to indicate the status of the
           internal state machine. When the BUSY line is held low, it
           indicates that the internal state machine is in idle mode and
           that the radio is ready to accept a command from the host
           controller. */
        wait_busy_pin_low(lora);
    }
    return true;
}
#if (LORA_DEBUG) && (LORA_DEBUG_SPI)
static inline void spi_dump(uint8_t* data, uint32_t size, bool sent)
{
    uint32_t i;
    printf("[sx126x] [info] spi data %s: ", sent ? "sent" : "rcvd");
    for (i = 0; i < size; ++i)
    {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
#endif
static inline void cmd_common(LORA* lora, uint8_t cmd, void* extra, uint32_t esize,
    void* data, uint32_t size, IO* io, bool write)
{
    uint32_t offset;
    uint8_t* iodata;
    LORA_SPI* spi;

    if (!wait_busy_pin(lora)) return;
    if (io) return; //in progress (need to create spi_read_sync_2(port, io1, io2) to avoid malloc+memcpy)

    spi     = &lora->spi;
    iodata  = (uint8_t*)io_data(spi->io);
    *iodata = cmd;
    offset  = 1;
    if (extra) { memcpy(iodata+offset, extra, esize); offset += esize; }
    if (!io)   { memcpy(iodata+offset, data,  size);  offset += size;  }
    spi->io->data_size = offset;

#if (LORA_DEBUG) && (LORA_DEBUG_SPI)
    //if (write)
        spi_dump(iodata, spi->io->data_size, true);
#endif

    gpio_reset_pin(lora->config.spi_cs_pin);
#if defined(STM32)
    //todo: could not use spi_send/spi_write_sync => use spi_read_sync instead
    if (write)
    {
                spi_read_sync (spi->port, spi->io, spi->io->data_size);
        if (io) spi_read_sync (spi->port,      io,      io->data_size);
    }
    else
    {
                spi_read_sync (spi->port, spi->io, spi->io->data_size);
        if (io) spi_read_sync (spi->port,      io,      io->data_size);
    }
#elif defined (MK22)
    if (write)
    {
                spi_write_sync(spi->port, spi->io);
        if (io) spi_write_sync(spi->port,      io);
    }
    else
    {
                spi_read_sync (spi->port, spi->io, spi->io->data_size);
        if (io) spi_read_sync (spi->port,      io,      io->data_size);
    }
#else
#error unsupported
#endif
    gpio_set_pin(lora->config.spi_cs_pin);

#if (LORA_DEBUG) && (LORA_DEBUG_SPI)
    //if (write)
        spi_dump(iodata, spi->io->data_size, false);
#endif

    offset = 1;
    if (extra)         { memcpy(extra, iodata+offset, esize); offset += esize; }
    //do not overwrite user data
    if (!io && !write) { memcpy(data,  iodata+offset, size); }
}
#if (CHECK_CMD_STATUS)
//Get status via get_cmd_status (spi transaction) and check it
static inline void check_cmd_status(LORA* lora, uint8_t cmd, bool write)
{
    //in progress...
}
#endif
static inline void cmd_write_extra(LORA* lora, uint8_t cmd, void* extra, uint32_t esize,
    void* data, uint32_t size)
{
    cmd_common(lora, cmd, extra, esize, data, size, 0, true);
#if (CHECK_CMD_STATUS)
    check_cmd_status(lora, cmd, true);
#endif
}
static inline void cmd_read_extra(LORA* lora, uint8_t cmd, void* extra, uint32_t esize,
    void* data, uint32_t size)
{
    cmd_common(lora, cmd, extra, esize, data, size, 0, false);
#if (CHECK_CMD_STATUS)
    check_cmd_status(lora, cmd, false);
#endif
}
static inline void cmd_write_extra_io(LORA* lora, uint8_t cmd, void* extra, uint32_t esize, IO* io)
{
    cmd_common(lora, cmd, extra, esize, 0, 0, io, true);
#if (CHECK_CMD_STATUS)
    check_cmd_status(lora, cmd, true);
#endif
}
static inline void cmd_read_extra_io(LORA* lora, uint8_t cmd, void* extra, uint32_t esize, IO* io)
{
    cmd_common(lora, cmd, extra, esize, 0, 0, io, false);
#if (CHECK_CMD_STATUS)
    check_cmd_status(lora, cmd, false);
#endif
}
static inline void cmd_write(LORA* lora, uint8_t cmd, void* data, uint32_t size)
{
    cmd_write_extra(lora, cmd, 0, 0, data, size);
}
static inline void cmd_read(LORA* lora, uint8_t cmd, void* data, uint32_t size)
{
    cmd_read_extra(lora, cmd, 0, 0, data, size);
}
/*
 * Table 11-5: Commands Returning the Radio Status
 */
//Returns the current status of the device
static inline void get_status(LORA* lora, uint8_t* status)
{
    //NOP
    *status = 0;
    cmd_read(lora, CMD_GET_STATUS, status, sizeof(*status));
}
static inline chip_mode_t get_chip_mode_ext(uint8_t status)
{
    return (status >> 4) & 7;
}
static inline cmd_status_t get_cmd_status_ext(uint8_t status)
{
    return (status >> 1) & 7;
}
static inline chip_mode_t get_chip_mode(LORA* lora)
{
    uint8_t status;
    get_status(lora, &status);
    return get_chip_mode_ext(status);
}
static inline cmd_status_t get_cmd_status(LORA* lora)
{
    uint8_t status;
    get_status(lora, &status);
    return get_cmd_status_ext(status);
}
//Returns the error which has occurred in the device
static inline void get_device_errors(LORA* lora, uint16_t* errors)
{
    //NOP
    uint8_t extra[] = {0};
    uint8_t args[] = {0, 0};
    cmd_read_extra(lora, CMD_GET_DEVICE_ERRORS, extra, sizeof(extra), args, sizeof(args));
    *errors = 0;
    *errors |= args[0] << 8;
    *errors |= args[1];
}
//Returns true if op_error set
static inline bool get_device_error(LORA* lora, op_error_t op_error)
{
    uint16_t errors;
    get_device_errors(lora, &errors);
    return (errors >> op_error) & 1;
}
//Clear all the error(s). The error(s) cannot be cleared independently
static inline void clear_device_errors(LORA* lora)
{
    uint8_t args[] = {0};
    cmd_write(lora, CMD_CLEAR_DEVICE_ERRORS, args, sizeof(args));
}
static inline state_t get_state_cur(LORA* lora)
{
    SYS_VARS* sys_vars = lora->sys_vars;
    //get cached value to avoid chip auto wake up due to spi transaction
    switch (sys_vars->chip_state_cached)
    {
    case ST_SLEEP_COLD_START:
        return ST_SLEEP_COLD_START;
    case ST_SLEEP_WARM_START:
        return ST_SLEEP_WARM_START;
    default:
        break;
    }
    switch (get_chip_mode(lora))
    {
    case CM_STBY_RC:
        return ST_STDBY_RC;
    case CM_STBY_XOSC:
        return ST_STDBY_XOSC;
    case CM_FS:
        return ST_FS;
    case CM_TX:
        return ST_TX;
    case CM_RX:
        return ST_RX;
    case CM_UNUSED:
    case CM_RFU:
    default:
        error (ERROR_NOT_SUPPORTED);
        return ST_UNKNOWN;
    }
    return ST_UNKNOWN;
}
//Set Chip in OM_SLEEP mode
/* The command SetSleep(...) is used to set the device in OM_SLEEP mode
with the lowest current consumption possible. This command can be
sent only while in STDBY mode (OM_STDBY_RC or OM_STDBY_XOSC). After the
rising edge of NSS, all blocks are switched OFF except the backup
regulator if needed and the blocks specified in the parameter
sleepConfig. */
static inline void set_sleep_ext(LORA* lora, sleep_config_t sleep_config)
{
    cmd_write(lora, CMD_SET_SLEEP, &sleep_config, sizeof(sleep_config));
    /* Caution:
       Once sending the command SetSleep(...), the device will become
       unresponsive for around 500 us, time needed for the configuration
       saving process and proper switch off of the various blocks. The user
       must thus make sure the device will not be receiving SPI command
       during these 500 us to ensure proper operations of the device. */
    //note: despite using wait_busy_pin we must sleep here (otherwise
    //      unexpected chip state is observed after ST_RX)
    //note: wait_busy_pin will _not_ cancel/negate waiting of these 500 us
    sleep_ms(1);
}
static inline void set_sleep(LORA* lora, bool warm, state_t state_cur)
{
    if (state_cur != SX126X_STDBY_TYPE)
    {
        set_state(lora, SX126X_STDBY_TYPE);
    }
    /* 9.3 Sleep Mode
       In this mode, most of the radio internal blocks are powered down or in
       low power mode and optionally the RC64k clock  and the timer are
       running. The chip may enter this mode from OM_STDBY_RC and can leave the
       OM_SLEEP mode if one of the following events occurs:
       - NSS pin goes low in any case
       - RTC timer generates an End-Of-Count (corresponding to Listen mode)
       When the radio is in Sleep mode, the BUSY pin is held high. */
    // RESERVED
    sleep_config_t sleep_config;
    sleep_config.SleepConfig_7_3 = 0;
    // 0: cold start,
    // 1: warm start (device configuration in retention)
    sleep_config.SleepConfig_2   = warm ? 1 : 0;
    // 0: RFU
    sleep_config.SleepConfig_1   = 0;
    // 0: RTC timeout disable
    // 1: wake-up on RTC timeout (RC64k)
    sleep_config.SleepConfig_0   = 0;
    set_sleep_ext(lora, sleep_config);
}
static inline void set_sleep_warm_start(LORA* lora, state_t state_cur)
{
    set_sleep(lora, true, state_cur);
}
static inline void set_sleep_cold_start(LORA* lora, state_t state_cur)
{
    set_sleep(lora, false, state_cur);
}
//Set Chip in OM_STDBY_RC or OM_STDBY_XOSC mode
static inline void set_standby(LORA* lora, standby_config_t standby_config)
{
    cmd_write(lora, CMD_SET_STANDBY, &standby_config, sizeof(standby_config));
}
//Set Chip in Freqency Synthesis mode
static inline void set_fs(LORA* lora)
{
    cmd_write(lora, CMD_SET_FS, 0, 0);
}
//note: if timeout_us > 4 sec function gives (0..+3) prec. error (due to use integer arith.),
//      which is eq. to 0..+46.875 error_us and usually can be neglected, because error_us << timeout_us
uint32_t get_timeout_23_0(uint32_t timeout_us)
{
    uint32_t timeout_23_0, div, mul;    int32_t perr;
    if (timeout_us < TIMEOUT_MIN_US)
    {
        timeout_us = TIMEOUT_MIN_US;
    }
    else if (timeout_us > TIMEOUT_MAX_US)
    {
        timeout_us = TIMEOUT_MAX_US;
    }
    /* The timeout duration can be computed with the formula:
       Timeout duration = Timeout * 15.625 us */
    div = mul = 1; perr = 0;
         if (timeout_us <=   4000000) { mul = 1000; div = 15625; perr = 0; }
    else if (timeout_us <=  42000000) { mul = 100;  div = 1562;  perr = PREC_ERROR_01(timeout_us); }
    else if (timeout_us <= 429000000) { mul = 10;   div = 156;   perr = PREC_ERROR_02(timeout_us); }
    timeout_23_0 = ((timeout_us * mul) / div) + perr;
    return timeout_23_0;
}
//Set Chip in Tx mode
static inline void set_tx(LORA* lora, uint32_t timeout_ms)
{
    uint32_t timeout_23_0;
    /* The timeout in Tx mode can be used as a security to ensure that if for
       any reason the Tx is aborted or does not succeed (ie. the TxDone IRQ
       never is never triggered), the TxTimeout will prevent the system from
       waiting for an unknown amount of time. Using the timeout while in Tx
       mode remove the need to use resources from the host MCU to perform the
       same task. */
    /* 0x000000 Timeout disable, Tx Single mode, the device will stay in
                TX Mode until the packet is transmitted and returns
                in STBY_RC mode upon completion.
       Others   Timeout active, the device remains in TX mode, it returns
                automatically to STBY_RC mode on timer end-of-count or when a packet
                has been transmitted. The maximum timeout is then 262 s. */
    if (timeout_ms == 0)
        timeout_23_0 = 0;
    else
        timeout_23_0 = get_timeout_23_0(timeout_ms*1000);

    uint8_t args[] = {
        (timeout_23_0 >> 16) & 0xff,
        (timeout_23_0 >> 8 ) & 0xff,
        (timeout_23_0 >> 0 ) & 0xff
    };
    cmd_write(lora, CMD_SET_TX, args, sizeof(args));
}
//Set Chip in Rx mode
static inline void set_rx(LORA* lora, uint32_t timeout_ms)
{
    uint32_t timeout_23_0;
    /* The receiver mode operates with a timeout to provide maximum flexibility to end users. */
    /* 0x000000 No timeout. Rx Single mode. The device will stay in RX
                Mode until a reception occurs and the devices return in STBY_RC mode
                upon completion
       0xFFFFFF Rx Continuous mode. The device remains in RX mode until
                the host sends a command to change the operation mode. The device can
                receive several packets. Each time a packet is received, a packet done
                indication is given to the host and the device will automatically
                search for a new packet.
       Others   Timeout active. The device remains in RX mode, it returns
                automatically to STBY_RC mode on timer end-of-count or when a packet
                has been received. As soon as a packet is detected, the timer is
                automatically disabled to allow complete reception of the packet.
                The maximum timeout is then 262 s. */
    /* NOTE1: "No timeout" (0x000000) looks useless cause it locks => not support it for now
       NOTE2: "SINGLE_RECEPTION"     in sx127x != "Rx Single mode"     in sx126x
                 ("SINGLE_RECEPTION" in sx127x == "Timeout active" in sx126x)
       NOTE3: "CONTINUOUS_RECEPTION" in sx127x == "Rx Continuous mode" in sx126x */
    if (timeout_ms == 0)
        timeout_23_0 = 0;
    else if (timeout_ms == TIMEOUT_MAX_VAL)
        timeout_23_0 = TIMEOUT_MAX_VAL;
    else
        timeout_23_0 = get_timeout_23_0(timeout_ms*1000);

    uint8_t args[] = {
        (timeout_23_0 >> 16) & 0xff,
        (timeout_23_0 >> 8 ) & 0xff,
        (timeout_23_0 >> 0 ) & 0xff
    };
    cmd_write(lora, CMD_SET_RX, args, sizeof(args));
}
static inline void set_reset(LORA* lora)
{
    LORA_CONFIG* config = &lora->config;
    /* A complete “factory reset” of the chip can be issued on
       request by toggling pin 15 NRESET of the SX1261/2. It will be
       automatically followed by the standard calibration procedure
       and any previous context will be lost. The pin should be held
       low for more than 50 μs (typically 100 μs) for the Reset to
       happen. */
    //todo reduce delay
    sleep_ms(20);
    gpio_reset_pin(config->reset_pin);
    sleep_ms(50);
    gpio_set_pin  (config->reset_pin);
    sleep_ms(20);
}
#if (LORA_DEBUG) && (LORA_DEBUG_STATE_CHANGES)
static inline void print_state_cur(LORA* lora)
{
    state_t state = get_state_cur(lora);
    printf("[sx126x] [info] state cur %u\n", state);
}
#endif
static inline void set_state(LORA* lora, state_t state_new, ...)
{
    state_t state_cur;
    va_list va;
    int32_t timeout_ms;
    SYS_VARS* sys_vars = lora->sys_vars;

    switch (state_new)
    {
        case ST_RESET:
            set_reset (lora);
            sys_vars->chip_state_cached = ST_RESET;
            return;
        case ST_STARTUP:
            //unsupported, internal state
            error (ERROR_NOT_SUPPORTED);
            return;
        default:
            break;
    }

    state_cur = get_state_cur(lora);

    if (state_cur == state_new)
    {
        //update in case of internal transitions (ex: ST_RESET->ST_STARTUP->ST_STDBY_RC)
        if (sys_vars->chip_state_cached != state_cur)
            sys_vars->chip_state_cached = state_cur;
        return;
    }

    switch (state_new)
    {
        case ST_SLEEP_COLD_START:
            set_sleep_cold_start(lora, state_cur);
            break;
        case ST_SLEEP_WARM_START:
            set_sleep_warm_start(lora, state_cur);
            break;
        case ST_STDBY_RC:
            set_standby(lora, CONF_STDBY_RC);
            break;
        case ST_STDBY_XOSC:
            set_standby(lora, CONF_STDBY_XOSC);
            break;
        case ST_FS:
            break;
        case ST_TX:
            va_start(va, state_new);
            timeout_ms = va_arg(va, uint32_t);
            va_end(va);
            set_tx(lora, timeout_ms);
            break;
        case ST_RX:
            va_start(va, state_new);
            timeout_ms = va_arg(va, uint32_t);
            va_end(va);
            set_rx(lora, timeout_ms);
            break;
        default:
            error (ERROR_NOT_SUPPORTED);
            break;
    }
    /*
     * NOTE: Do not wait specific delay from Table 8-2: Switching Time
     *       due to wait busy in spi write/read
     */
    sys_vars->chip_state_cached = state_new;

#if (LORA_DEBUG) && (LORA_DEBUG_STATE_CHANGES)
    //any spi trans in sleep state will cause wakeup => avoid these states
    if (state_new != ST_SLEEP_COLD_START && state_new != ST_SLEEP_WARM_START)
    {
        cmd_status_t cmd_status = get_cmd_status(lora);
        printf("[sx126x] [info] cmd_status %u\n", cmd_status);
    }
    print_state_cur(lora);
#endif
}
//Stop Rx timeout on Sync Word/Header or preamble detection
static inline void stop_timer_on_preamble(LORA* lora, stop_on_preamble_param_t stop_on_preamble_param)
{
    cmd_write(lora, CMD_STOP_TIMER_ON_PREAMBLE, &stop_on_preamble_param, sizeof(stop_on_preamble_param));
}
//Store values of RTC setup for listen mode and if period parameter is not 0, set chip into RX mode
static inline void set_rx_duty_cycle(LORA* lora, uint32_t rx_period_23_0, uint32_t sleep_period_23_0)
{
    uint8_t args[] = {
        (rx_period_23_0    >> 16) & 0xff,
        (rx_period_23_0    >> 8 ) & 0xff,
        (rx_period_23_0    >> 0 ) & 0xff,
        (sleep_period_23_0 >> 16) & 0xff,
        (sleep_period_23_0 >> 8 ) & 0xff,
        (sleep_period_23_0 >> 0 ) & 0xff,
    };
    cmd_write(lora, CMD_SET_RX_DUTY_CYCLE, args, sizeof(args));
}
//Set chip into RX mode with passed CAD parameters
static inline void set_cad(LORA* lora)
{
    cmd_write(lora, CMD_SET_CAD, 0, 0);
}
//Set chip into TX mode with infinite carrier wave settings
static inline void set_tx_continuous_wave(LORA* lora)
{
    cmd_write(lora, CMD_SET_TX_CONTINUOUS_WAVE, 0, 0);
}
//Set chip into TX mode with infinite preamble settings
static inline void set_tx_infinite_preamble(LORA* lora)
{
    cmd_write(lora, CMD_SET_TX_INFINITE_PREAMBLE, 0, 0);
}
//Select LDO or DC_DC+LDO for CFG_XOSC, OM_FS, RX or TX mode
static inline void set_regulator_mode(LORA* lora)
{
    regulator_mode_t mode = SX126X_REGULATOR_MODE;
    cmd_write(lora, CMD_SET_REGULATOR_MODE, &mode, sizeof(mode));
}
//Calibrate the RC13, RC64, ADC, PLL, Image according to parameter
static inline void calibrate(LORA* lora, calib_param_t calib_param)
{
    cmd_write(lora, CMD_CALIBRATE, &calib_param, sizeof(calib_param));
}
//Launches an image calibration at the given frequencies
static inline void calibrate_image(LORA* lora)
{
    uint8_t args[] = {FREQ1, FREQ2};
    cmd_write(lora, CMD_CALIBRATE_IMAGE, args, sizeof(args));
}
//Configure the Duty Cycle, Max output power, device for the PA for SX1261 or SX1262
static inline void set_pa_config(LORA* lora, uint8_t pa_duty_cycle, uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut)
{
    uint8_t args[] = {pa_duty_cycle, hp_max, device_sel, pa_lut};
    cmd_write(lora, CMD_SET_PA_CONFIG, args, sizeof(args));
}
//Defines into which mode the chip goes after a TX / RX done.
static inline void set_rx_tx_fallback_mode(LORA* lora, fallback_mode_t fallback_mode)
{
    cmd_write(lora, CMD_SET_RX_TX_FALLBACK_MODE, &fallback_mode, sizeof(fallback_mode));
}
/*
 * Table 11-2: Commands to Access the Radio Registers and FIFO Buffer
 */
//Write into one or several registers
static inline void write_reg_common(LORA* lora, uint16_t address_15_0, uint8_t* data, uint32_t size)
{
    uint8_t extra[] = {
        (address_15_0 >> 8) & 0xff,
        (address_15_0 >> 0) & 0xff
    };
    cmd_write_extra(lora, CMD_WRITE_REGISTER, extra, sizeof(extra), data, size);
}
//Read one or several registers
static inline void read_reg_common(LORA* lora, uint16_t address_15_0, uint8_t* data, uint32_t size)
{
    uint8_t extra[] = {
        (address_15_0 >> 8) & 0xff,
        (address_15_0 >> 0) & 0xff,
        /* Note that the host has to send an NOP after sending the 2
           bytes of address to start receiving data bytes on the next
           NOP sent. */
        0
    };
    cmd_read_extra(lora, CMD_READ_REGISTER, extra, sizeof(extra), data, size);
}
static inline void write_reg(LORA* lora, uint16_t address_15_0, uint8_t data)
{
    write_reg_common(lora, address_15_0, &data, 1);
}
static inline void read_reg(LORA* lora, uint16_t address_15_0, uint8_t* data)
{
    read_reg_common(lora, address_15_0, data, 1);
}
//Write data into the FIFO
static inline void write_buffer(LORA* lora, uint8_t offset, IO* io)
{
    uint8_t extra[] = {offset};
    //cmd_write_extra_io(lora, CMD_WRITE_BUFFER, extra, sizeof(extra), io);
    cmd_write_extra(lora, CMD_WRITE_BUFFER, extra, sizeof(extra), io_data(io), io->data_size);
}
//Read data from the FIFO
static inline void read_buffer(LORA* lora, uint8_t offset, IO* io)
{
    //NOP
    uint8_t extra[] = {offset, 0};
    //cmd_read_extra_io(lora, CMD_READ_BUFFER, extra, sizeof(extra), io);
    cmd_read_extra(lora, CMD_READ_BUFFER, extra, sizeof(extra), io_data(io), io->data_size);
}
/*
 * Table 11-3: Commands Controlling the Radio IRQs and DIOs
 */
//Configure the IRQ and the DIOs attached to each IRQ
static inline void set_dio_irq_params(LORA* lora)
{
    uint8_t args[] = {
        (SX126X_IRQ_MASK  >> 8) & 0xff,
        (SX126X_IRQ_MASK  >> 0) & 0xff,
        (SX126X_DIO1_MASK >> 8) & 0xff,
        (SX126X_DIO1_MASK >> 0) & 0xff,
        (SX126X_DIO2_MASK >> 8) & 0xff,
        (SX126X_DIO2_MASK >> 0) & 0xff,
        (SX126X_DIO3_MASK >> 8) & 0xff,
        (SX126X_DIO3_MASK >> 0) & 0xff,
    };
    cmd_write(lora, CMD_SET_DIO_IRQ_PARAMS, args, sizeof(args));
}
//Get the values of the triggered IRQs
static inline void get_irq_status(LORA* lora, /*uint8_t status,*/ uint16_t* irq_status_15_0)
{
    uint8_t extra[] = {0};
    uint8_t args[] = {0, 0};
    cmd_read_extra(lora, CMD_GET_IRQ_STATUS, extra, sizeof(extra), args, sizeof(args));
    *irq_status_15_0 = 0;
    *irq_status_15_0 |= args[0] << 8;
    *irq_status_15_0 |= args[1];
    //*irq_status_15_0 &= 0x3FF;
}
static inline bool is_irq_set(LORA* lora, irq_t irq, uint16_t irq_status)
{
    return (irq_status >> irq) & 1;
}
//Clear one or several of the IRQs
static inline void clear_irq_status(LORA* lora, uint16_t clear_irq_param_15_0)
{
    /* This function clears an IRQ flag in the IRQ register by setting to 1
       the bit of ClearIrqParam corresponding to the same position as the IRQ
       flag to be cleared. As an example, if bit 0 of ClearIrqParam is set
       to 1 then IRQ flag at bit 0 of IRQ register is cleared. */
    uint8_t args[] = {
        (clear_irq_param_15_0 >> 8) & 0xff,
        (clear_irq_param_15_0 >> 0) & 0xff
    };
    cmd_write(lora, CMD_CLEAR_IRQ_STATUS, args, sizeof(args));
}
static inline void clear_irq_status_all(LORA* lora)
{
    clear_irq_status(lora, 0xFFFF);
}
static inline void clear_irq_status_irq(LORA* lora, irq_t irq)
{
    uint16_t clear_irq_param_15_0 = 0;
    clear_irq_param_15_0 |= (1 << irq);
    clear_irq_status(lora, clear_irq_param_15_0);
}
//Configure radio to control an RF switch from DIO2
static inline void set_dio2_as_rf_switch_ctrl(LORA* lora)
{
    uint8_t enable = 1;
    cmd_write(lora, CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &enable, 1);
}
//Configure the radio to use a TCXO controlled by DIO3
static inline bool set_dio3_as_tcxo_ctrl(LORA* lora)
{
    uint32_t timeout_23_0 = get_timeout_23_0(SX126X_OSC_32MHZ_STABILIZE_TIMEOUT_MS*1000);
    uint8_t args[] = {
        SX126X_TCXO_VOLTAGE,
        /* The time needed for the 32 MHz to appear and stabilize can be
           controlled through the parameter timeout. */
        (timeout_23_0 >> 16) & 0xff,
        (timeout_23_0 >> 8 ) & 0xff,
        (timeout_23_0 >> 0 ) & 0xff,
    };
#if (LORA_DEBUG)
    //clear_device_errors(lora);
#endif
    cmd_write(lora, CMD_SET_DIO3_AS_TCXO_CTRL, args, sizeof(args));
    /* If the 32 MHz from the TCXO is not detected internally at the end the
       timeout period, the error XOSC_START_ERR will be flagged in the error
       controller. */
#if (LORA_DEBUG)
    //todo: issue: XOSC_START_ERR is set even after clear_device_errors...
    //if (get_device_error(lora, OE_XOSC_START_ERR))
    //{
    //    printf("[sx126x] [error] 32 MHz from the TCXO is not detected internally\n");
    //    return false;
    //}
#endif
    return true;
}
static inline uint32_t int_pow(int base, int exp)
{
    uint32_t r = 1;
    while (exp--)
       r *= base;
    return r;
}
/*
 * Table 11-4: Commands Controlling the RF and Packets Settings
 */
//Set the RF frequency of the radio (rf_frequency_mhz: 430..928 MHz (Table 9-2))
static inline void set_rf_frequency_mhz(LORA* lora)
{
    //FREQ_XTAL   32000000
    //FREQ_DIV    33554432
    //FREQ_STEP = FREQ_XTAL/FREQ_DIV = 0.95367431640625
    //original formula: rf_freq = ((double)rf_frequency_mhz / (double)FREQ_STEP)

    //number of decimal digits in the output rf_freq
    const uint32_t digits = 9;
    //for max prec. with uint32_t (up to max freq 928 MHz)
    const uint32_t mul = 4;
    //manually decrease prec. (otherwise dividend*10 exceeds uint32_t)
    const uint32_t divisor = (953674316 * mul) / 10;

    uint32_t dividend = LORA_RF_CARRIER_FREQUENCY_MHZ * 1000000 * mul;
    uint32_t quotient = 0, digit, i, rf_freq;
    uint8_t args[4];

    //progressively get digits (based on long division)
    for (i = 1; i <= digits; ++i)
    {
        digit    = dividend / divisor;
        quotient += (digit * int_pow(10, digits-i));
        dividend = (dividend - (digit * divisor)) * 10;
    }

    //manually increase prec. (if need)
    quotient += PREC_ERROR_03(quotient);

    rf_freq = quotient;

    args[0] = (rf_freq >> 24) & 0xff;
    args[1] = (rf_freq >> 16) & 0xff;
    args[2] = (rf_freq >>  8) & 0xff;
    args[3] = (rf_freq >>  0) & 0xff;
    cmd_write(lora, CMD_SET_RF_FREQUENCY, args, sizeof(args));
}
//Select the packet type corresponding to the modem
static inline void set_packet_type(LORA* lora, packet_type_t packet_type)
{
    cmd_write(lora, CMD_SET_PACKET_TYPE, &packet_type, sizeof(packet_type));
}
//Get the current packet configuration for the device
static inline void get_packet_type(LORA* lora, uint8_t* packet_type)
{
    uint8_t extra[] = {0};      *packet_type = 0;
    cmd_read_extra(lora, CMD_GET_PACKET_TYPE, extra, sizeof(extra), packet_type, sizeof(*packet_type));
}
//Set output power and ramp time for the PA
static inline bool set_tx_params(LORA* lora)
{
    uint8_t args[] = {OUTPUT_POWER_DBM, SX126X_RAMP_TIME};
    /* The output power is defined as power in dBm in a range of
       -17 (0xEF) to +14 (0x0E) dBm by step of 1 dB if low  power PA is selected
       -9  (0xF7) to +22 (0x16) dBm by step of 1 dB if high power PA is selected */
    /* Selection between high power PA and low power PA
       NOTE: By default low power PA and +14 dBm are set. */
    /* Mode  Output Power  paDutyCycle  hpMax  deviceSel  paLut  Value in SetTxParams
     SX1261  +15 dBm       0x06         0x00   0x01       0x01   +14 dBm
             +14 dBm       0x04         0x00   0x01       0x01   +14 dBm
             +10 dBm       0x01         0x00   0x01       0x01   +13 dBm */
    /* set_pa_config arguments:
       - paDutyCycle controls the duty cycle (conduction angle) of both PAs (SX1261 and SX1262).
       - hpMax selects the size of the PA in the SX1262, this value has no influence on the SX1261
       - deviceSel is used to select either the SX1261 or the SX1262.
       - paLut is reserved and has always the value 0x01. */
    //NOTE: The SX1261 transceiver is capable of outputting +14/15 dBm maximum
    //      output power under DC-DC converter or LDO supply.
    if      (LORA_PA_SELECT==0 && (OUTPUT_POWER_DBM >= -17 && OUTPUT_POWER_DBM <= 14))
    {
        set_pa_config(lora, 0x04, 0, SX1261_PA_CONFIG_DEV_SEL, 0x01);
    }
    else if (LORA_PA_SELECT==1 && (OUTPUT_POWER_DBM >= -9  && OUTPUT_POWER_DBM <= 15))
    {
        set_pa_config(lora, 0x06, 0, SX1261_PA_CONFIG_DEV_SEL, 0x01);
    }
    else
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }

    /* Set the Over Current Protection level. The value is changed
       internally depending on the device selected. Default values
       are: SX1262: 0x38 (140 mA) SX1261: 0x18 (60 mA) */
    write_reg(lora, REG_OCP_CONFIGURATION, 0x18);
    cmd_write(lora, CMD_SET_TX_PARAMS, args, sizeof(args));
    return true;
}
//Compute and set values in selected protocol modem for given modulation parameters
static inline void set_modulation_params(LORA* lora)
{
    uint8_t args[] = {
        LORA_SPREADING_FACTOR,
        LORA_SIGNAL_BANDWIDTH,
        LORA_CODING_RATE,
        LOW_DATA_RATE_OPTIMIZE
    };
    cmd_write(lora, CMD_SET_MODULATION_PARAMS, args, sizeof(args));
}
//Set values on selected protocol modem for given packet parameters
static inline void set_packet_params_ext(LORA* lora,
    uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6)
{
    uint8_t args[] = {p1, p2, p3, p4, p5, p6};
    cmd_write(lora, CMD_SET_PACKET_PARAMS, args, sizeof(args));
}
//NOTE: to set just payload_length need to re-set all params... => wrap into set_packet_params_ext
static inline void set_packet_params_pl(LORA* lora, uint8_t payload_length)
{
    uint8_t p[6];
    /* Table 13-66: LoRa® PacketParam1 & PacketParam2 - PreambleLength
       PreambleLength (15:0)   Description
       0x0001 to 0xFFFF        preamble length: number of symbols sent as preamble */
    /* The transmitted preamble length may vary from 10 to 65535 symbols */
    p[0] = (LORA_PREAMBLE_LEN >> 8) & 0xff;
    p[1] = (LORA_PREAMBLE_LEN >> 0) & 0xff;
    /* Table 13-67: LoRa® PacketParam3 - HeaderType
       HeaderType   Description
       0x00         Variable length packet (explicit header)
       0x01         Fixed length packet (implicit header) */
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    p[2] = LORA_IMPLICIT_HEADER_MODE_ON;
    /* Table 13-68: LoRa® PacketParam4 - PayloadLength
       Payloadlength   Description
       0x00 to 0xFF    Size of the payload (in bytes) to transmit or maximum size of the
                       payload that the receiver can accept. */
    p[3] = payload_length;
    /* Table 13-69: LoRa® PacketParam5 - CRCType
       CRCType Description
       0x00    CRC OFF
       0x01    CRC ON */
    p[4] = LORA_RX_PAYLOAD_CRC_ON;
    /* Table 13-70: LoRa® PacketParam6 - InvertIQ
       AddrComp    Description
       0x00        Standard IQ setup
       0x01        Inverted IQ setup */
    p[5] = LORA_INVERT_IQ;
    set_packet_params_ext(lora, p[0], p[1], p[2], p[3], p[4], p[5]);
}
static inline void set_packet_params(LORA* lora)
{
    set_packet_params_pl(lora, 0);
}
static inline void set_payload_length(LORA* lora, uint8_t payload_length)
{
    set_packet_params_pl(lora, payload_length);
}
//Set the parameters which are used for performing a CAD (LoRa® only)
static inline void set_cad_params(LORA* lora, uint8_t cad_symbol_num,
    uint8_t cad_det_peak, uint8_t cad_det_min, uint8_t cad_exit_mode, uint8_t cad_timeout)
{
    uint8_t args[] = {cad_symbol_num, cad_det_peak, cad_det_min, cad_exit_mode, cad_timeout};
    cmd_write(lora, CMD_SET_CAD_PARAMS, args, sizeof(args));
}
//Store TX and RX base address in register of selected protocol modem
static inline void set_buffer_base_address(LORA* lora)
{
    uint8_t args[] = {FIFO_TX_BASE_ADDR, FIFO_RX_BASE_ADDR};
    cmd_write(lora, CMD_SET_BUFFER_BASE_ADDRESS, args, sizeof(args));
}
//Set the number of symbol the modem has to wait to validate a lock
static inline void set_symb_num_timeout(LORA* lora, uint8_t symb_num)
{
    cmd_write(lora, CMD_SET_LORA_SYMB_NUM_TIMEOUT, &symb_num, sizeof(symb_num));
}
//Returns the instantaneous measured RSSI while in Rx mode
static inline void get_rssi_inst(LORA* lora, uint8_t* rssi)
{
    //NOP
    uint8_t extra[] = {0};
    cmd_read_extra(lora, CMD_GET_RSSI_INST, extra, sizeof(extra), rssi, sizeof(*rssi));
}
//Returns PaylaodLengthRx(7:0), RxBufferPointer(7:0)
static inline void get_rx_buffer_status(LORA* lora,
    uint8_t* paylaod_length_rx, uint8_t* rx_buffer_pointer)
{
    //NOP
    uint8_t extra[] = {0};
    uint8_t args[] = {0, 0};
    cmd_read_extra(lora, CMD_GET_RX_BUFFER_STATUS, extra, sizeof(extra), args, sizeof(args));
    *paylaod_length_rx = args[0];
    *rx_buffer_pointer = args[1];
}
static inline uint8_t get_payload_length_rx(LORA* lora)
{
    uint8_t paylaod_length_rx, unused;
    get_rx_buffer_status(lora, &paylaod_length_rx, &unused);
    return paylaod_length_rx;
}
/* Returns RssiPkt, SnrPkt, SignalRssiPkt
   RssiPkt - Average over last packet received of RSSI (Actual signal power is –RssiPkt/2 (dBm))
   SnrPkt  - Estimation of SNR on last packet received.In two’s compliment format multiplied by 4.
             (Actual SNR in dB =SnrPkt/4)
   SignalRssiPkt - Estimation of RSSI of the LoRa® signal (after despreading) on last packet received.
                   In two’s compliment format. (Actual Rssi in dB = -SignalRssiPkt/2) */
static inline void get_packet_status(LORA* lora, uint8_t* rssi_pkt, uint8_t* snr_pkt, uint8_t* signal_rssi_pkt)
{
    //NOP
    uint8_t extra[] = {0};
    uint8_t args[] = {0, 0, 0};
    cmd_read_extra(lora, CMD_GET_PACKET_STATUS, extra, sizeof(extra), args, sizeof(args));
    *rssi_pkt        = args[0];
    *snr_pkt         = args[1];
    *signal_rssi_pkt = args[2];
}
//Returns statistics on the last few received packets
static inline void get_stats(LORA* lora, stats_t* stats)
{
    cmd_read(lora, CMD_GET_STATS, stats, sizeof(*stats));
}
//Resets the value read by the command GetStats
//todo: optionally call from loras_clear_stats
static inline void reset_stats(LORA* lora)
{
    /* This command resets the value read by the command GetStats. To execute this command,
       the opcode is 0x0 followed by 6 zeros (so 7 zeros in total) */
    uint8_t args[] = {0, 0, 0, 0, 0, 0};
    cmd_write(lora, CMD_RESET_STATS, args, sizeof(args));
}
static inline uint32_t get_bandwidth_hz(LORA* lora)
{
    switch (LORA_SIGNAL_BANDWIDTH)
    {
    case LORA_BW_500:
        return 500000;
    case LORA_BW_250:
        return 250000;
    case LORA_BW_125:
        return 125000;
    case LORA_BW_062:
        return 62500;
    case LORA_BW_041:
        return 41670;
    case LORA_BW_031:
        return 31250;
    case LORA_BW_020:
        return 20830;
    case LORA_BW_015:
        return 15630;
    case LORA_BW_010:
        return 10420;
    case LORA_BW_007:
        return 7810;
    default:
        error (ERROR_NOT_SUPPORTED);
        return 0;
    }
    return 0;
}
static inline uint8_t get_payload_length(LORA* lora)
{
    uint8_t payload_length;

    if (lora->tx || LORA_IMPLICIT_HEADER_MODE_ON)
    {
        //todo: no way to get payload_length from the chip via SPI??
        payload_length = lora->io->data_size;
    }
    else
    {
        payload_length = get_payload_length_rx(lora);
    }
    return payload_length;
}
//note: returns predefined value to make crc flag independent of tx flag (see get_pkt_time_on_air_ms)
static inline bool crc_is_present_in_rcvd_pkt_header(LORA* lora)
{
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    //note: could not find in the sx126x data sheet how to obtain CRC presence
    //      from header when explicit header mode is used
    //note: in sx127x CrcOnPayload is used
    //solution: CRC is always present in the header
    return LORA_RX_PAYLOAD_CRC_ON;
}
#if (LORA_DEBUG) && (LORA_DEBUG_DO_SPI_TEST)
static inline void spi_test(LORA* lora)
{
    const int test_sel = 2, iter = 255;
    uint8_t reg_data = 0, cnt = 0, i;
    chip_mode_t chip_mode;

    for (i = 0; i < iter; ++i)
    {
        if (test_sel == 0)
        {
            chip_mode = get_chip_mode (lora);
            printf("[sx126x] [info] chip_mode %d\n", chip_mode);
            sleep_ms(1000);
        }
        else if (test_sel == 1) //sleep test
        {
            printf("[sx126x] [info] before ST_SLEEP_WARM_START\n");
            set_state(lora, ST_SLEEP_WARM_START);
            printf("[sx126x] [info] after  ST_SLEEP_WARM_START\n\n");
            sleep_ms(1000);
            printf("[sx126x] [info] before ST_STDBY_RC\n");
            set_state(lora, ST_STDBY_RC);
            printf("[sx126x] [info] after  ST_STDBY_RC\n\n");
            sleep_ms(1000);
        }
        else if (test_sel == 2)
        {
            //0x14 expected
            read_reg(lora, REG_LORA_SYNC_WORD_MSB, &reg_data);
            printf("[sx126x] [info] reg_data %x\n", reg_data);
            //set_packet_type (lora, PACKET_TYPE_LORA);
            //get_packet_type (lora, &reg_data);
            sleep_ms(1000);
            //continue;

            write_reg(lora, REG_LORA_SYNC_WORD_MSB, cnt++);
            sleep_ms(1000);
            //print_state_cur(lora);
            if (cnt >= 255) cnt = 0;
        }
    }
}
#endif

static inline void clear_sys_vars(SYS_VARS* sys_vars)
{
    memset(sys_vars, 0, sizeof(SYS_VARS));
    sys_vars->chip_state_cached = ST_UNKNOWN;
}

static inline bool set_sys_vars(LORA* lora)
{
    if (lora->sys_vars == NULL)
    {
        lora->sys_vars = malloc(sizeof(SYS_VARS));
        if (lora->sys_vars == NULL)
        {
            error(ERROR_OUT_OF_MEMORY);
            return false;
        }
        clear_sys_vars(lora->sys_vars);
    }
    return true;
}

static inline bool hw_reset(LORA* lora)
{
#if (LORA_DEBUG)
    state_t state;
#endif
    //POR & manual reset
    if (LORA_DO_MANUAL_RESET_AT_OPEN)
    {
        set_state(lora, ST_RESET);
#if (LORA_DEBUG)
        printf("[sx126x] [info] manual reset completed\n");
#endif
        /* 1. Calibration procedure is automatically called in case of POR
           2. Once the calibration is finished, the chip enters OM_STDBY_RC mode
           3. By default, after battery insertion or reset operation (pin NRESET
              goes low), the chip will enter in OM_STDBY_RC mode running with a 13
              MHz RC clock. */
        /* Once the calibration is finished, the chip enters OM_STDBY_RC mode. */
#if (LORA_DEBUG)
        state = get_state_cur(lora);
        if (state != SX126X_STDBY_TYPE)
        {
            error(ERROR_INVALID_STATE);
            return false;
        }
#endif
    }
    else
    {
        /* 1.   If not in OM_STDBY_RC mode, then go to this mode with the command SetStandby(...) */
        /* 13.1.2 SetStandby
           The command SetStandby(...) is used to set the device in a
           configuration mode which is at an intermediate level of consumption.
           In this mode, the chip is placed in halt mode waiting for instructions
           via SPI. This mode is dedicated to chip configuration using high
           level commands such as SetPacketType(...). */
        set_state(lora, SX126X_STDBY_TYPE);
    }
    return true;
}

bool loras_hw_open(LORA* lora)
{
    if (!set_sys_vars(lora))
    {
        return false;
    }

    if (!hw_reset(lora))
    {
        return false;
    }

#if (LORA_DEBUG) && (LORA_DEBUG_DO_SPI_TEST)
    spi_test(lora);
#endif

    /* DIO2 can be configured to drive an RF switch through the use
       of the command SetDio2AsRfSwitchCtrl(...). In this mode, DIO2
       will be at a logical 1 during Tx and at a logical 0 in any
       other mode */
    if (SX126X_DIO2_AS_RF_SWITCH_CTRL)
    {
        set_dio2_as_rf_switch_ctrl(lora);
    }

    /* DIO3 can be used to automatically control a TCXO through the command
       SetDio3AsTCXOCtrl(...). In this case, the device will automatically power
       cycle the TCXO when needed. */
    if (SX126X_DIO3_AS_TCXO_CTRL)
    {
        if (!set_dio3_as_tcxo_ctrl(lora))
        {
            return false;
        }
    }

    /* 1. Calibration procedure is automatically called in case of POR
       2. Once the calibration is finished, the chip enters OM_STDBY_RC mode
       3. By default, after battery insertion or reset operation (pin NRESET
          goes low), the chip will enter in OM_STDBY_RC mode running with a 13
          MHz RC clock. */
    /* 9.2 Calibrate
       Calibration procedure is automatically called in case of POR or via
       the calibration command. Parameters can be added to the calibrate
       command to identify which section of calibration should be repeated.
       The following blocks can be calibrated:
       - RC64k using the 32 MHz crystal oscillator as reference
       - RC13M using the 32 MHz crystal oscillator as reference
       - PLL to select the proper VCO frequency and division ratio for any RF frequency
       - RX ADC
       - Image (RX mode with defined tone)
       Once the calibration is finished, the chip enters OM_STDBY_RC mode. */
    if (SX126X_DO_MANUAL_CALIBRATION)
    {
        calib_param_t calib_param;
        calib_param.bit_0_rc64k_calibration_enabled      = 1;
        calib_param.bit_1_rc13m_calibration_enabled      = 1;
        calib_param.bit_2_pll_calibration_enabled        = 1;
        calib_param.bit_3_adc_pulse_calibration_enabled  = 1;
        calib_param.bit_4_adc_bulk_n_calibration_enabled = 1;
        calib_param.bit_5_adc_bulk_p_calibration_enabled = 1;
        calib_param.bit_6_image_calibration_enabled      = 1;
        calib_param.bit_7_rfu                            = 0;
        calibrate(lora, calib_param);

        /* 9.2.1 Image Calibration for Specific Frequency Bands
           The image calibration is done through the command CalibrateImage(...)
           for a given range of frequencies defined by the parameters freq1 and
           freq2. Once performed, the calibration is valid for all frequencies
           between the two extremes used as parameters. Typically, the user can
           select the parameters freq1 and freq2 to cover any specific ISM band. */
        calibrate_image(lora);
    }

    //configure modem

    /* 2.   Define the protocol (LoRa® or FSK) with the command SetPacketType(...) */
    /* The user specifies the modem and frame type by using the command
       SetPacketType(...). This command specifies the frame used and
       consequently the modem implemented.
       This function is the first one to be called before going to Rx or Tx
       and before defining modulation and packet parameters. The command
       GetPacketType() returns the current protocol of the radio. */
    set_packet_type(lora, PACKET_TYPE_LORA);

    /* 3.   Define the RF frequency with the command SetRfFrequency(...) */
    set_rf_frequency_mhz(lora);

    /* 4.   Define output power and ramping time with the command SetTxParams(...) */
    //NOTE: will be done in loras_hw_tx_async

    /* 5.   Define where the data payload will be stored with the command SetBufferBaseAddress(...) */
    set_buffer_base_address(lora);

    /* 6.   Send the payload to the data buffer with the command WriteBuffer(...) */
    //NOTE: will be done in loras_hw_tx_async

    /* 7.   Define the modulation parameter according to the chosen protocol with the command
            SetModulationParams(...) */
    /* SF is defined by the parameter Param[1]
       BW is defined by the parameter Param[2]
       CR is defined by the parameter Param[3] */
    /* The parameter LdOpt corresponds to the Low Data Rate Optimization (
       LDRO). This parameter is usually set when the LoRa® symbol time is
       equal or above 16.38 ms (typically for SF11  with  BW125  and  SF12
       with  BW125  and  BW250).  See  Section 6.1.1.4 "Low Data Rate
       Optimization" on page 39. */

    set_modulation_params(lora);

    /* 8.   Define the frame format to be used with the command SetPacketParams(...) */
    //NOTE: need pl for time_on_air_ms calc. (to check if timer ms values are reasonable)
    set_packet_params(lora);

    /* 9.   Configure DIO and IRQ: use the command SetDioIrqParams(...) to select TxDone IRQ and
            map this IRQ to a DIO (DIO1, DIO2 or DIO3) */
    /* By default, all IRQ are masked (all ‘0’) and the user
       can enable them one by one (or several at a time) by
       setting the corresponding mask to ‘1’.*/
    //todo: need mask for the user??
    set_dio_irq_params(lora);

    /* 10.  Define Sync Word value: use the command WriteReg(...) to write the value of the register
            via direct register access */
    write_reg(lora, REG_LORA_SYNC_WORD_MSB, (SX126X_SYNC_WORD >> 8) & 0xff);
    write_reg(lora, REG_LORA_SYNC_WORD_LSB, (SX126X_SYNC_WORD >> 0) & 0xff);

	  //Select LDO or DC_DC+LDO for CFG_XOSC, OM_FS, RX or TX mode
    set_regulator_mode(lora);

    //enter sleep mode
    set_state(lora, ST_SLEEP_WARM_START);

    return true;
}

void loras_hw_close(LORA* lora)
{
    if (get_state_cur(lora) != SX126X_STDBY_TYPE)
    {
        set_state(lora, SX126X_STDBY_TYPE);
    }
    set_state(lora, ST_SLEEP_COLD_START);
}

void loras_hw_sleep(LORA* lora)
{
    state_t state = get_state_cur(lora);
    if (state != ST_SLEEP_COLD_START || state != ST_SLEEP_WARM_START)
    {
        if (state != SX126X_STDBY_TYPE)
        {
            set_state(lora, SX126X_STDBY_TYPE);
        }
        set_state(lora, ST_SLEEP_WARM_START);
    }
}

void loras_hw_tx_async_wait(LORA* lora)
{
    uint16_t irq_status;
#if (LORA_DEBUG)
    state_t state;
#endif

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    //NOTE: Do not know if packet is already sent (state TX or STDBY)
    //      => do not check state
#endif

    /* 12.  Wait for the IRQ TxDone or Timeout: once the packet has been sent the chip goes
            automatically to OM_STDBY_RC mode */
    get_irq_status(lora, &irq_status);
    if (is_irq_set(lora, IRQ_TX_DONE, irq_status))
    {
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else if (is_irq_set(lora, IRQ_TIMEOUT, irq_status))
    {
        error(ERROR_TIMEOUT);
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
        set_state(lora, ST_SLEEP_WARM_START);
#if (LORA_DO_ERROR_COUNTING)
        ++lora->stats_tx.timeout_num;
#endif
        return;
    }
    else
    {
        //no irq
        error(ERROR_OK);
        return;
    }

#if (LORA_DEBUG)
    // chip auto-goes back to STDBY
    state = get_state_cur(lora);
    if (state != SX126X_STDBY_TYPE)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
#endif

    /* 13.  Clear the IRQ TxDone flag */
    //clear_irq_status_irq(lora, TX_DONE);
    clear_irq_status_all(lora);

    // Go back from OM_STDBY_RC to OM_SLEEP
    set_state(lora, ST_SLEEP_WARM_START);
    error(ERROR_OK);
}

void loras_hw_tx_async(LORA* lora, IO* io)
{
#if (LORA_DEBUG)
    state_t state;
#endif
    /* 1.   If not in OM_STDBY_RC mode, then go to this mode with the command SetStandby(...) */
    /* 13.1.2 SetStandby
       The command SetStandby(...) is used to set the device in a
       configuration mode which is at an intermediate level of consumption.
       In this mode, the chip is placed in halt mode waiting for instructions
       via SPI. This mode is dedicated to chip configuration using high
       level commands such as SetPacketType(...). */
#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_COMPLETED)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
#endif

    //stop RX (if was previously started)
    if (LORA_CONTINUOUS_RECEPTION_ON && lora->rxcont_mode_started)
    {
#if (LORA_DEBUG)
        printf("[sx126x] [info] new lora_tx while RX is ON (continuous reception) => stop RX\n");
#endif
        // Go back to OM_SLEEP
        set_state(lora, ST_SLEEP_WARM_START);
        lora->rxcont_mode_started = false;
    }

#if (LORA_DEBUG)
    state = get_state_cur(lora);
    if (state != ST_SLEEP_COLD_START && state != ST_SLEEP_WARM_START)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
#endif
    set_state(lora, SX126X_STDBY_TYPE);

    /* 4.   Define output power and ramping time with the command SetTxParams(...) */
    if (!set_tx_params(lora))
    {
        return;
    }

    /* 6.   Send the payload to the data buffer with the command WriteBuffer(...) */
    /* The parameter offset defines the address pointer of the first data to be
       written or read. Offset zero defines the first position of the data buffer. */
    write_buffer(lora, FIFO_TX_BASE_ADDR, io);

    // Set payload length as size of current data
    set_payload_length(lora, io->data_size);

    // clear all irqs
    clear_irq_status_all(lora);

    /* 11.  Set the circuit in transmitter mode to start transmission with the command SetTx().
            Use the parameter to enable Timeout */
    set_state(lora, ST_TX, lora->tx_timeout_ms);

    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    error(ERROR_OK);
}

void loras_hw_rx_async_wait(LORA* lora)
{
#if (LORA_DEBUG)
    state_t state;
#endif
    uint16_t irq_status;
    uint8_t payload_length_rx, rx_start_buffer_pointer;

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    state = get_state_cur(lora);
    if (LORA_CONTINUOUS_RECEPTION_ON)
    {
        if (state != ST_RX)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
    }
    //else => Single mode: the device returns automatically to OM_STDBY_RC mode after packet reception
    //NOTE: Do not know if packet is received (state is RX or STDBY)
    //      => do not check state
#endif

    /* 10.  Wait for IRQ RxDone or Timeout: the chip will stay in Rx and look
       for a new packet if the continuous mode is selected otherwise it will
       goes to OM_STDBY_RC mode. */
    get_irq_status(lora, &irq_status);
    // wait for RX_DONE IRQ
    if (is_irq_set(lora, IRQ_RX_DONE, irq_status))
    {
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else if (!LORA_CONTINUOUS_RECEPTION_ON && is_irq_set(lora, IRQ_TIMEOUT, irq_status))
    {
        error(ERROR_TIMEOUT);
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
        set_state(lora, ST_SLEEP_WARM_START);
#if (LORA_DO_ERROR_COUNTING)
        ++lora->stats_rx.timeout_num;
#endif
        return;
    }
    else
    {
        //no irq
        error(ERROR_OK);
        return;
    }

    /* 11.  In case of the IRQ RxDone, check the status to ensure CRC is correct:
       use the command GetIrqStatus()
       The IRQ RxDone means that a packet has been received but the CRC could be
       wrong: the user must check the CRC before validating the packet. */
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    if (LORA_IMPLICIT_HEADER_MODE_ON || crc_is_present_in_rcvd_pkt_header(lora))
    {
        if (is_irq_set(lora, IRQ_CRC_ERR, irq_status))
        {
            error(LORA_ERROR_RX_INCORRECT_PAYLOAD_CRC);
            lora->status = LORA_STATUS_TRANSFER_COMPLETED;
            if (!LORA_CONTINUOUS_RECEPTION_ON)
                set_state(lora, ST_SLEEP_WARM_START);
#if (LORA_DO_ERROR_COUNTING)
            ++lora->stats_rx.crc_err_num;
#endif
            return;
        }
    }

    // clear all irqs
    clear_irq_status_all(lora);

    /* In case of a valid packet (CRC Ok), get the packet length and
       address of the first byte of the received payload by using
       the command GetRxBufferStatus(...) */
    /* 13.  In case of a valid packet (CRC Ok), start reading the packet */
    get_rx_buffer_status(lora, &payload_length_rx, &rx_start_buffer_pointer);
    if (payload_length_rx > 0)
    {
        lora->io->data_size = payload_length_rx;
        read_buffer(lora, rx_start_buffer_pointer, lora->io);
    }
    else
    {
        error(LORA_ERROR_RX_INCORRECT_PAYLOAD_LEN);
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
        if (!LORA_CONTINUOUS_RECEPTION_ON)
            set_state(lora, ST_SLEEP_WARM_START);
#if (LORA_DO_ERROR_COUNTING)
        //tbd
#endif
        lora->io->data_size = 0;
        return;
    }

    if (!LORA_CONTINUOUS_RECEPTION_ON)
    {
        // Go back to OM_SLEEP
        set_state(lora, ST_SLEEP_WARM_START);
    }
    /* else => LORA_CONTINUOUS_RECEPTION
       0xFFFFFF Rx Continuous mode. The device remains in RX mode until
       the host sends a command to change the operation mode. The device can
       receive several packets. Each time a packet is received, a packet done
       indication is given to the host and the device will automatically
       search for a new packet. */
    error(ERROR_OK);
}

void loras_hw_rx_async(LORA* lora, IO* io)
{
    uint8_t start_rx;
    uint32_t timeout;
#if (LORA_DEBUG)
    state_t state;
#endif

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_COMPLETED)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    state = get_state_cur(lora);
    if (LORA_CONTINUOUS_RECEPTION_ON && lora->rxcont_mode_started)
    {
        if (state != ST_RX)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
    }
    else
    {
        if (state != ST_SLEEP_COLD_START && state != ST_SLEEP_WARM_START)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
    }
#endif

    /* Payload length in bytes. The register needs to be set in
       implicit header mode for the expected packet length.
       A 0 value is not permitted */
    if (LORA_IMPLICIT_HEADER_MODE_ON)
    {
        set_payload_length(lora, io->data_size);
    }

    start_rx = 0;

    /* NOTE1: "No timeout" (0x000000) looks useless cause it locks => not support it for now
       NOTE2: "SINGLE_RECEPTION"     in sx126x != "Rx Single mode"     in sx126x
                 ("SINGLE_RECEPTION" in sx126x == "Timeout active" in sx126x)
       NOTE3: "CONTINUOUS_RECEPTION" in sx126x == "Rx Continuous mode" in sx126x */
    if (!LORA_CONTINUOUS_RECEPTION_ON)
    {
        set_state(lora, SX126X_STDBY_TYPE);
        start_rx = 1;
        /* By default, the timer will be stopped only if the Sync Word or header
           has been detected. However, it is also possible to stop the timer upon
           preamble detection by using the command StopTimerOnPreamble(...). */
        //NOTE: this is to make behavior for the end user similar between sx126x and sx126x
        //      => stop the timer upon preamble detection
        if (lora->rx_timeout_ms < TIMEOUT_MAX_VAL)
        {
            stop_timer_on_preamble(lora, STOP_ON_PREAMBLE_ENABLE);
        }
    }
    else if (!lora->rxcont_mode_started)
    {
        set_state(lora, SX126X_STDBY_TYPE);
        start_rx = 1;
        lora->rxcont_mode_started = true;
    }

    // clear all irqs
    clear_irq_status_all(lora);		
		
    if (start_rx)
    {
#if (LORA_DEBUG)
        state = get_state_cur(lora);
        if (state != SX126X_STDBY_TYPE)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
#endif

        /* 9. Set the circuit in reception mode: use the command SetRx().
              Set the parameter to enable timeout or continuous mode */
        /* The receiver mode operates with a timeout to provide maximum flexibility to end users. */
        /* 0x000000 No timeout. Rx Single mode. The device will stay in RX
                    Mode until a reception occurs and the devices return in STBY_RC mode
                    upon completion
           0xFFFFFF Rx Continuous mode. The device remains in RX mode until
                    the host sends a command to change the operation mode. The device can
                    receive several packets. Each time a packet is received, a packet done
                    indication is given to the host and the device will automatically
                    search for a new packet.
           Others   Timeout active. The device remains in RX mode, it returns
                    automatically to STBY_RC mode on timer end-of-count or when a packet
                    has been received. As soon as a packet is detected, the timer is
                    automatically disabled to allow complete reception of the packet.
                    The maximum timeout is then 262 s. */
        /* NOTE1: "No timeout" (0x000000) looks useless cause it locks => not support it for now
           NOTE2: "SINGLE_RECEPTION"     in sx127x != "Rx Single mode"     in sx126x
                 ("SINGLE_RECEPTION"     in sx127x == "Timeout active"     in sx126x)
           NOTE3: "CONTINUOUS_RECEPTION" in sx127x == "Rx Continuous mode" in sx126x */
        timeout = 0;
        if (LORA_CONTINUOUS_RECEPTION_ON)
        {
            timeout = 0xFFFFFF;
        }
        else
        {
            timeout = lora->rx_timeout_ms;
        }
        set_state(lora, ST_RX, timeout);
    }

    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    error(ERROR_OK);
    //NOTE: software timer_txrx_timeout is used only for LORA_CONTINUOUS_RECEPTION
    //      (for LORA_SINGLE_RECEPTION internal rx_timeout_ms is used).
    if (LORA_CONTINUOUS_RECEPTION_ON)
    {
        timer_start_ms(lora->timer_txrx_timeout, lora->rx_timeout_ms);
    }
}
static inline uint32_t max(int32_t a, int32_t b)
{
    return a > b ? a : b;
}
static inline uint32_t ceil(uint32_t exp, uint32_t fra)
{
    return (fra > 0) ? exp+1 : exp;
}
//NOTE: based on sx126x time-on-air calculation + adapted for sx126x
static inline uint32_t get_pkt_time_on_air_ms_common(LORA* lora/*, bool tx*/, uint8_t pl)
{
    uint32_t bw, sf, r_s, t_s, /*pl,*/ ih, de, crc, cr, exp, fra, dividend, divisor;
    uint32_t n_preamble, n_payload, t_preamble, t_payload, t_packet;

    bw = get_bandwidth_hz(lora);
    sf = LORA_SPREADING_FACTOR;
    n_preamble = LORA_PREAMBLE_LEN;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * DOMAIN_US) / r_s;
    // preamble duration
    t_preamble = (n_preamble + 4.25) * t_s;

    // -- where PL is the number of bytes of payload
    /*pl = get_payload_length(lora);*/
    // -- SF is the spreading factor
    // -- IH = 1 when implicit header mode is enabled and IH = 0 when explicit header mode is used
    ih = LORA_IMPLICIT_HEADER_MODE_ON;
    // -- DE set to 1 indicates the use of the low data rate optimization, 0 when disabled
    de = LOW_DATA_RATE_OPTIMIZE;
    // -- CRC indicates the presence of the payload CRC = 1 when on 0 when off
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    /* In Explicit header mode: CRC recovered from the header */
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    if (ih/* || tx*/)
    {
        crc = LORA_RX_PAYLOAD_CRC_ON;
    }
    else
    {
        crc = crc_is_present_in_rcvd_pkt_header(lora);
    }

    // -- CR is the programmed coding rate from 1 to 4.
    cr = LORA_CODING_RATE;

    //see 6.1.4 LoRa Time-on-Air
    if (sf == 5 || sf == 6)
    {
        dividend = max((8*pl) + (16*crc) - (4*sf) + (20*ih), 0);
        divisor  = (4*sf);
    }
    else if (de == 0)
    {
        dividend = max((8*pl) + (16*crc) - (4*sf) + 8 + (20*ih), 0);
        divisor  = (4*sf);
    }
    else
    {
        dividend = max((8*pl) + (16*crc) - (4*sf) + 8 + (20*ih), 0);
        divisor  = (4*(sf-2));
    }

    exp = dividend / divisor;
    //get 1st digit of fractional part
    fra = ((dividend * 10) / divisor) - (exp * 10);

    // number of payload symbols
    if (sf == 5 || sf == 6)
    {
        n_payload = 6.25 + 8 + ceil(exp, fra) * (cr+4);
    }
    else
    {
        n_payload = 4.25 + 8 + ceil(exp, fra) * (cr+4);
    }
    // payload duration
    t_payload = n_payload * t_s;
    // total packet time on air
    t_packet = t_preamble + t_payload;
    return t_packet / 1000;
}
static inline uint32_t get_pkt_time_on_air_ms(LORA* lora)
{
    return get_pkt_time_on_air_ms_common(lora, get_payload_length(lora));
}
static inline uint32_t get_pkt_time_on_air_max_ms(LORA* lora)
{
    return get_pkt_time_on_air_ms_common(lora, LORA_MAX_PAYLOAD_LEN);
}
static inline int8_t get_tx_output_power_dbm(LORA* lora)
{
    //TX output power
    return OUTPUT_POWER_DBM;
}
static inline int16_t get_rssi_cur_dbm(LORA* lora)
{
    uint8_t rssi;
    //Returns the instantaneous measured RSSI while in Rx mode
    get_rssi_inst(lora, &rssi);
    return -(rssi/2);
}
static inline int16_t get_rssi_last_pkt_rcvd_dbm(LORA* lora)
{
//Returns RssiPkt, SnrPkt, SignalRssiPkt
//RssiPkt - Average over last packet received of RSSI (Actual signal power is –RssiPkt/2 (dBm))
//SnrPkt  - Estimation of SNR on last packet received. In two’s compliment format multiplied by 4.
//          (Actual SNR in dB =SnrPkt/4)
//SignalRssiPkt - Estimation of RSSI of the LoRa® signal (after despreading) on last packet received.In two’s compliment format.[negated, dBm, fixdt(0,8,1)]
//                (Actual Rssi in dB = -SignalRssiPkt/2)
    uint8_t rssi_pkt, snr_pkt, signal_rssi_pkt;
    get_packet_status(lora, &rssi_pkt, &snr_pkt, &signal_rssi_pkt);
    return -(signal_rssi_pkt/2);
}
static inline int8_t get_snr_last_pkt_rcvd_db(LORA* lora)
{
    uint8_t rssi_pkt, snr_pkt, signal_rssi_pkt;
    get_packet_status(lora, &rssi_pkt, &snr_pkt, &signal_rssi_pkt);
    return snr_pkt/4;
}

static inline int32_t get_rf_freq_error_hz(LORA* lora)
{
    uint8_t regval;
    int32_t f_err_i;
    uint32_t f_err = 0;
    read_reg(lora, REG_FREQ_ERROR+0, &regval); f_err |= (regval << 16);
    read_reg(lora, REG_FREQ_ERROR+1, &regval); f_err |= (regval << 8);
    read_reg(lora, REG_FREQ_ERROR+2, &regval); f_err |= (regval << 0);
    //NOTE: REG_FREQ_ERROR (0x076B) seems not documented => cast it to int32_t...
    f_err_i = (int32_t)f_err;
    return f_err_i;
}

uint32_t loras_hw_get_stat(LORA* lora, LORA_STAT stat_)
{
    state_t state;
    uint32_t stat;
    bool was_sleep = false;

    state = get_state_cur(lora);
    if (state == ST_SLEEP_COLD_START || state == ST_SLEEP_WARM_START)
    {
        was_sleep = true;
        /* In Sleep mode, the BUSY pin is held high through a 20 kΩ resistor
           and the BUSY line will go low as soon as the radio leaves the Sleep
           mode. */
        //wake-up chip (will set BUSY to low)
        set_state(lora, SX126X_STDBY_TYPE);
    }

    switch (stat_)
    {
    case PKT_TIME_ON_AIR_MS:
        stat = /*uint32_t*/get_pkt_time_on_air_ms(lora);
        break;
    case PKT_TIME_ON_AIR_MAX_MS:
        stat = /*uint32_t*/get_pkt_time_on_air_max_ms(lora);
        break;
    case TX_OUTPUT_POWER_DBM:
        stat = /*  int8_t*/get_tx_output_power_dbm(lora);
        break;
    case RX_RSSI_CUR_DBM:
        stat = /* int16_t*/get_rssi_cur_dbm(lora);
        break;
    case RX_RSSI_LAST_PKT_RCVD_DBM:
        stat = /* int16_t*/get_rssi_last_pkt_rcvd_dbm(lora);
        break;
    case RX_SNR_LAST_PKT_RCVD_DB:
        stat = /*  int8_t*/get_snr_last_pkt_rcvd_db(lora);
        break;
    case RX_RF_FREQ_ERROR_HZ:
        stat = /* int32_t*/get_rf_freq_error_hz(lora);
        break;
    default:
        stat = 0;
        error (ERROR_NOT_SUPPORTED);
        break;
    }

    if (was_sleep)
    {
        /* In Sleep mode, the BUSY pin is held high through a 20 kΩ resistor
           and the BUSY line will go low as soon as the radio leaves the Sleep
           mode. */
        set_state(lora, ST_SLEEP_WARM_START);
    }

    return stat;
}


