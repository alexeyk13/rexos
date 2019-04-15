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

//---------- local settings depending on LORA_xx settings and/or LORA_CHIP -----------
#if (LORA_CHIP == SX1276)
#if (LORA_RF_CARRIER_FREQUENCY_MHZ <= 525)
//Sets the floor reference for all AGC thresholds:
//  AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
//  SNR = 8dB, fixed value
#define AGC_REFERENCE_LEVEL                                 0x19
//Defines the Nth AGC Threshold
#define AGC_STEP1                                           0x0c
#define AGC_STEP2                                           0x04
#define AGC_STEP3                                           0x0b
#define AGC_STEP4                                           0x0c
#define AGC_STEP5                                           0x0c
//Controls the PLL bandwidth:
//  00 - 75 kHz    10 - 225 kHz    01 - 150 kHz    11 - 300 kHz
#define PLL_BANDWIDTH                                       0x03
#elif (LORA_RF_CARRIER_FREQUENCY_MHZ >= 862)
#define AGC_REFERENCE_LEVEL                                 0x1c
#define AGC_STEP1                                           0x0e
#define AGC_STEP2                                           0x05
#define AGC_STEP3                                           0x0b
#define AGC_STEP4                                           0x0c
#define AGC_STEP5                                           0x0c
#define PLL_BANDWIDTH                                       0x03
#endif
#endif

#if (LORA_CHIP == SX1272)
//Power amplifier output power:
//  Pout =  2 + OutputPower(3:0) on PA_BOOST
//  Pout = -1 + OutputPower(3:0) on RFIO
#define OUTPUT_POWER                                        15
#elif (LORA_CHIP == SX1276)
//Power amplifier output power:
//  Pout = Pmax - (15-OutputPower) if PaSelect = 0 (RFO pin)
//  Pout = 17   - (15-OutputPower) if PaSelect = 1 (PA_BOOST pin)
#define OUTPUT_POWER                                        15
//Select max output power: Pmax=10.8 + 0.6 * MaxPower [dBm]
#if (SX127X_ENABLE_HIGH_OUTPUT_POWER)
#define MAX_POWER                                           0x7
#else
#define MAX_POWER                                           0x4
#endif
#endif

//LowDataRateOptimize: 0 = Disabled     1 = Enabled
//SX1272: LDO is mandated for SF11 and SF12 with BW = 125 kHz
//SX1276: LDO is mandated for when the symbol length exceeds 16 ms (runtime check)
#if (LORA_CHIP == SX1272 && \
    ((LORA_SPREADING_FACTOR == 11 || LORA_SIGNAL_BANDWIDTH == 0) || \
     (LORA_SPREADING_FACTOR == 12 || LORA_SIGNAL_BANDWIDTH == 0)))
#define LOW_DATA_RATE_OPTIMIZE                              1
#else
#define LOW_DATA_RATE_OPTIMIZE                              0
#endif

//SF = 6 is a special use case for the highest data rate
#if (LORA_SPREADING_FACTOR == 6)
//detection optimize:  0x03 = SF7 to SF12    0x05 = SF6
#define DETECTION_OPTIMIZE                                  0x05
//detection threshold: 0x0A = SF7 to SF12    0x0C = SF6
#define DETECTION_THRESHOLD                                 0x0C
#else
#define DETECTION_OPTIMIZE                                  0x03
#define DETECTION_THRESHOLD                                 0x0A
#endif

//check config correctness
#include "check_config.h"
//-------------------- local settings depending on LORA_xx settings (end) ------------

#define POR_RESET_MS                                        10
//SPI interface address pointer in FIFO data buffer
#define FIFO_ADDR_PTR                                       0x00
//Write base address in FIFO data buffer for TX modulator
#define FIFO_TX_BASE_ADDR                                   0x00
//Read  base address in FIFO data buffer for RX demodulator
#define FIFO_RX_BASE_ADDR                                   0x00

/* Semtech ID relating the silicon revision */
#define REG_VERSION                                         0x42
#define REG_FIFO                                            0x00
/* Common Register Settings */
#define REG_OP_MODE                                         0x01
#define REG_FR_MSB                                          0x06
#define REG_FR_MIB                                          0x07
#define REG_FR_LSB                                          0x08
/* Low/High Frequency Additional Registers - SX1276 specific */
#define REG_AGC_REF_LF                                      0x61
#define REG_AGC_THRESH1_LF                                  0x62
#define REG_AGC_THRESH2_LF                                  0x63
#define REG_AGC_THRESH3_LF                                  0x64
#define REG_PLL_LF                                          0x70
/* Register for RF */
#define REG_PA_CONFIG                                       0x09
#define REG_PA_RAMP                                         0x0A
#define REG_OCP                                             0x0B
#define REG_LNA                                             0x0C
/* Lora page registers */
#define REG_FIFO_ADDR_PTR                                   0x0D
#define REG_FIFO_TX_BASE_ADDR                               0x0E
#define REG_FIFO_RX_BASE_ADDR                               0x0F
#define REG_FIFO_RX_CURRENT_ADDR                            0x10
#define REG_IRQ_FLAGS_MASK                                  0x11
#define REG_IRQ_FLAGS                                       0x12
#define REG_RX_NB_BYTES                                     0x13
#define REG_RX_HEADER_CNT_VALUE_MSB                         0x14
#define REG_RX_HEADER_CNT_VALUE_LSB                         0x15
#define REG_RX_PACKET_CNT_VALUE_MSB                         0x16
#define REG_RX_PACKET_CNT_VALUE_LSB                         0x17
#define REG_MODEM_STAT                                      0x18
#define REG_PKT_SNR_VALUE                                   0x19
#define REG_PKT_RSSI_VALUE                                  0x1A
#define REG_RSSI_VALUE                                      0x1B
#define REG_HOP_CHANNEL                                     0x1C
#define REG_MODEM_CONFIG1                                   0x1D
#define REG_MODEM_CONFIG2                                   0x1E
#define REG_MODEM_CONFIG3                                   0x26
#define REG_PPM_CORRECTION                                  0x27
#define REG_SYMB_TIMEOUT_LSB                                0x1F
#define REG_PREAMBLE_MSB                                    0x20
#define REG_PREAMBLE_LSB                                    0x21
#define REG_PAYLOAD_LENGTH                                  0x22
#define REG_MAX_PAYLOAD_LENGTH                              0x23
#define REG_HOP_PERIOD                                      0x24
#define REG_FIFO_RX_BYTE_ADDR                               0x25
#define REG_FEI_MSB                                         0x28
#define REG_FEI_MID                                         0x29
#define REG_FEI_LSB                                         0x2A
#define REG_RSSI_WIDEBAND                                   0x2C
#define REG_DETECT_OPTIMIZE                                 0x31
#define REG_INVERT_I_Q                                      0x33
#define REG_DETECTION_THRESHOLD                             0x37
#define REG_SYNC_WORD                                       0x39
#define SX1272_REG_PA_DAC                                   0x5A
#define SX1276_REG_PA_DAC                                   0x4D

/* RegSymbTimeout register and should be in the range of 4 (minimum time for the
   modem to acquire lock on a preamble) up to 1023 symbols. */
#define TIMEOUT_MIN_SYMB                                    4
#define TIMEOUT_MAX_SYMB                                    1023

//to keep precision with integer arith.
#define DOMAIN_US                                           1000000

typedef struct {
    /* 7.2.1. POR reset */
    bool        por_completed;
    //add more (if need)
} SYS_VARS;

//Device modes
typedef enum
{
    /* Low-power mode. In this mode only SPI and configuration registers are accessible.
       Lora FIFO is not accessible. Note that this is the only mode permissible to switch
       between FSK/OOK mode and LoRa mode. */
    SLEEP = 0,
    /* Both crystal oscillator and LoRa baseband blocks are turned on.
       RF front-end and PLLs are disabled. */
    STDBY,
    /* This is a frequency synthesis mode for transmission. The PLL selected for transmission
       is locked and active at the transmit frequency. The RF front-end is off. */
    FSTX,
    /* TX When activated the SX1272/73 powers all remaining blocks required for transmit,
       ramps the PA, transmits the packet and returns to Standby mode. */
    TX,
    /* This is a frequency synthesis mode for reception. The PLL selected for reception
       is locked and active at the receive frequency. The RF front-end is off. */
    FSRX,
    /* When activated the SX1272/73 powers all remaining blocks required for reception,
       processing all received data until a new user request is made to change operating mode. */
    RXCONTINUOUS,
    /* When activated the SX1272/73 powers all remaining blocks required for reception,
       remains in this state until a valid packet has been received and then returns to Standby mode. */
    RXSINGLE,
    /* When in CAD mode, the device will check a given channel to detect LoRa preamble signal. */
    CAD
} mode_t;

typedef enum
{
    FSK_OOK_MODE = 0,
    LORA_MODE
} long_range_mode_t;

typedef enum
{
    IRQ_CAD_DETECTED = 0,
    IRQ_FHSS_CHANGE_CHANNEL,
    IRQ_CAD_DONE,
    IRQ_TX_DONE,
    IRQ_VALID_HEADER,
    IRQ_PAYLOAD_CRC_ERROR,
    /* At the end of the payload, the RxDone interrupt is generated together with
       the interrupt PayloadCrcError if the payload CRC is not valid. */
    IRQ_RX_DONE,
    /* The modem searches for a preamble during a given period of time. If a preamble hasn’t
       been found at the end of the time window, the chip generates the RxTimeout interrupt
       and goes back to Standby mode. The length of the reception window (in symbols) is
       defined by the RegSymbTimeout register and should be in the range of 4 (minimum time
       for the modem to acquire lock on a preamble) up to 1023 symbols. */
    IRQ_RX_TIMEOUT
} irq_t;

// Several forward declarations needed
static inline uint8_t get_mode(LORA* lora);
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
static inline void write_reg_common(LORA* lora, uint8_t address, uint8_t* data, uint16_t size)
{
    LORA_SPI* spi = &lora->spi;
    ((uint8_t*)io_data(spi->io))[0] = address | 0x80;
    memcpy(io_data(spi->io)+1, data, size);
    spi->io->data_size = size+1;

    gpio_reset_pin(lora->config.spi_cs_pin);
#if defined(STM32)
    //todo: could not use spi_send/spi_write_sync => use spi_read_sync instead
    spi_read_sync(spi->port, spi->io, spi->io->data_size);
#elif defined (MK22)
    spi_write_sync(spi->port, spi->io);
#else
#error "unknown target"
#endif
    gpio_set_pin(lora->config.spi_cs_pin);
}
static inline void write_reg(LORA* lora, uint8_t address, uint8_t data)
{
    write_reg_common(lora, address, &data, 1);
}
static inline void write_reg_io(LORA* lora, uint8_t address, IO* io)
{
    LORA_SPI* spi = &lora->spi;
    ((uint8_t*)io_data(spi->io))[0] = address | 0x80;
    spi->io->data_size = 1;

    gpio_reset_pin(lora->config.spi_cs_pin);
#if defined(STM32)
    //todo: could not use spi_send/spi_write_sync => use spi_read_sync instead
    spi_read_sync (spi->port, spi->io, spi->io->data_size);
    spi_read_sync (spi->port,      io,      io->data_size);
#elif defined (MK22)
    spi_write_sync(spi->port, spi->io);
    spi_write_sync(spi->port,      io);
#else
#error unsupported
#endif
    gpio_set_pin(lora->config.spi_cs_pin);
}
static inline void read_reg_common(LORA* lora, uint8_t address, uint8_t* data, uint16_t size)
{
    LORA_SPI* spi = &lora->spi;
    ((uint8_t*)io_data(spi->io))[0] = address & 0x7F;
    spi->io->data_size = size+1;

    gpio_reset_pin(lora->config.spi_cs_pin);
    spi_read_sync(spi->port, spi->io, spi->io->data_size);
    gpio_set_pin(lora->config.spi_cs_pin);

    memcpy(data, io_data(spi->io)+1, size);
}
static inline uint8_t read_reg(LORA* lora, uint8_t address)
{
    uint8_t reg;
    read_reg_common(lora, address, &reg, 1);
    return reg;
}
static inline void read_reg_io(LORA* lora, uint8_t address, IO* io)
{
    LORA_SPI* spi = &lora->spi;
    ((uint8_t*)io_data(spi->io))[0] = address & 0x7F;
    spi->io->data_size = 1;

    gpio_reset_pin(lora->config.spi_cs_pin);
    spi_read_sync (spi->port, spi->io, spi->io->data_size);
    spi_read_sync (spi->port,      io,      io->data_size);
    gpio_set_pin  (lora->config.spi_cs_pin);
#if (LORA_DEBUG) && (LORA_DEBUG_SPI)
    spi_dump(io_data(io), io->data_size, false);
#endif
}
/* Function setbits(x,p,n,y) that returns x with the n bits that
   begin at position p set to the rightmost n bits of y, leaving
   the other bits unchanged.*/
static inline unsigned setbits(unsigned x, int p, int n, unsigned y)
{
    int shift = p - n + 1;
    unsigned mask = (1 << n) - 1;
    return ((((x >> shift) ^ y) & mask) << shift) ^ x;
}
static inline uint8_t get_version(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_VERSION);
    return reg;
}
static inline bool write_fifo(LORA* lora, IO* io)
{
#if (LORA_DEBUG)
    /* FIFO is cleared an not accessible when device is in SLEEP mode */
    if (get_mode(lora) == SLEEP)
    {
        error(ERROR_INVALID_STATE);
        return false;
    }
#endif
    //write_reg_io(lora, REG_FIFO, io);
    write_reg_common(lora, REG_FIFO, io_data(io), io->data_size);
    return true;
}
static inline bool read_fifo(LORA* lora, IO* io)
{
    /* FIFO is cleared an not accessible when device is in SLEEP mode */
    if (get_mode(lora) == SLEEP)
    {
        error(ERROR_INVALID_STATE);
        return false;
    }
    //read_reg_io(lora, REG_FIFO, io);
    read_reg_common(lora, REG_FIFO, io_data(io), io->data_size);
    return true;
}
static inline void set_mode(LORA* lora, uint8_t mode)
{
    uint8_t reg;
    reg = read_reg(lora, REG_OP_MODE);
    /* Device modes */
    reg = setbits(reg, 2, 3, mode);
    write_reg(lora, REG_OP_MODE, reg);
}
static inline uint8_t get_mode(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_OP_MODE);
    return (reg >> 0) & 7;
}
static inline void set_long_range_mode(LORA* lora, uint8_t long_range_mode)
{
    uint8_t reg;
    reg = read_reg(lora, REG_OP_MODE);
    /* This bit can be modified only in Sleep mode. A write operation on
        other device modes is ignored. */
    if (get_mode(lora) == SLEEP) {
        /* LongRangeMode:
            0 - FSK/OOK Mode
            1 - LoRaTM Mode */
        reg = setbits(reg, 7, 1, long_range_mode);
        write_reg(lora, REG_OP_MODE, reg);
    }
}
static inline uint8_t get_long_range_mode(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_OP_MODE);
    /* LongRangeMode:
        0 - FSK/OOK Mode
        1 - LoRaTM Mode */
    return (reg >> 7) & 1;
}
static inline void set_access_shared_reg(LORA* lora, uint8_t access_shared_reg)
{
    uint8_t reg;
    reg = read_reg(lora, REG_OP_MODE);
    /* This bit operates when device is in Lora mode; if set it allows
        access to FSK registers page located in address space
        (0x0D:0x3F) while in LoRa mode.
        0 - Access LoRa registers page 0x0D: 0x3F
        1 - Access FSK registers page (in mode LoRa) 0x0D: 0x3F */
    if (get_long_range_mode(lora) == LORA_MODE) {
        reg = setbits(reg, 6, 1, access_shared_reg);
        write_reg(lora, REG_OP_MODE, reg);
    }
}
static inline uint8_t get_access_shared_reg(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_OP_MODE);
    return (reg >> 6) & 1;
}
static inline void set_low_frequency_mode_on(LORA* lora, uint8_t low_frequency_mode_on)
{
    uint8_t reg;
    reg = read_reg(lora, REG_OP_MODE);
    /* Access Low Frequency Mode registers
        0 - High Frequency Mode (access to HF test registers)
        1 - Low  Frequency Mode (access to LF test registers) */
    reg = setbits(reg, 3, 1, low_frequency_mode_on);
    write_reg(lora, REG_OP_MODE, reg);
}
static inline uint8_t get_low_frequency_mode_on(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_OP_MODE);
    /* Access Low Frequency Mode registers
        0 - High Frequency Mode (access to HF test registers)
        1 - Low Frequency Mode (access to LF test registers) */
    return (reg >> 3) & 1;
}
static inline void set_rf_carrier_frequency(LORA* lora)
{
    uint32_t value, freq_osc_mhz, mode;
    /* Resolution is 61.035 Hz if F(XOSC) = 32 MHz. Default value is
        0xe4c000 = 915 MHz. Register values must be modified only
        when device is in SLEEP or STANDBY mode. */
    mode = get_mode(lora);
    if (mode == SLEEP || mode == STDBY)
    {
        //Crystal oscillator frequency (see Table 7 Frequency Synthesizer Specification)
        freq_osc_mhz = 32;
        value = (LORA_RF_CARRIER_FREQUENCY_MHZ * (1<<19)) / freq_osc_mhz;
        write_reg(lora, REG_FR_MSB, (value >> 16) & 0xff);
        write_reg(lora, REG_FR_MIB, (value >> 8) & 0xff);
        write_reg(lora, REG_FR_LSB, value & 0xff);
    }
    else
    {
#if (LORA_DEBUG)
        printf("[sx127x] [error] cannot set RF carrier frequency (device mode %u)\n", mode);
#endif
    }
}
static inline uint32_t get_rf_carrier_frequency(LORA* lora)
{
    uint32_t value = 0;
    value |= read_reg(lora, REG_FR_LSB);
    value |= read_reg(lora, REG_FR_MIB) << 8;
    value |= read_reg(lora, REG_FR_MSB) << 16;
    return value;
}
#if (LORA_CHIP == SX1276)
static inline void set_frequency_additional_registers(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_AGC_REF_LF);
    /* Sets the floor reference for all AGC thresholds:
       AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
       SNR = 8dB, fixed value */
    reg = setbits(reg, 5, 6, AGC_REFERENCE_LEVEL);
    write_reg(lora, REG_AGC_REF_LF, reg);

    reg = read_reg(lora, REG_AGC_THRESH1_LF);
    /* Defines the 1st AGC Threshold */
    reg = setbits(reg, 4, 5, AGC_STEP1);
    write_reg(lora, REG_AGC_THRESH1_LF, reg);

    reg = read_reg(lora, REG_AGC_THRESH2_LF);
    /* Defines the 2nd AGC Threshold */
    reg = setbits(reg, 7, 4, AGC_STEP2);
    /* Defines the 3rd AGC Threshold */
    reg = setbits(reg, 3, 4, AGC_STEP3);
    write_reg(lora, REG_AGC_THRESH2_LF, reg);

    reg = read_reg(lora, REG_AGC_THRESH3_LF);
    /* Defines the 4th AGC Threshold */
    reg = setbits(reg, 7, 4, AGC_STEP4);
    /* Defines the 5th AGC Threshold */
    reg = setbits(reg, 3, 4, AGC_STEP5);
    write_reg(lora, REG_AGC_THRESH3_LF, reg);

    reg = read_reg(lora, REG_PLL_LF);
    /* Controls the PLL bandwidth:
        00 - 75 kHz     10 - 225 kHz
        01 - 150 kHz    11 - 300 kHz */
    reg = setbits(reg, 7, 2, PLL_BANDWIDTH);
    write_reg(lora, REG_PLL_LF, reg);
}
#endif
/* Low/High Frequency Additional Registers - SX1276 specific */
static inline void get_frequency_additional_registers(LORA* lora,
    uint8_t* agc_reference_level,
    uint8_t* agc_step1,
    uint8_t* agc_step2,
    uint8_t* agc_step3,
    uint8_t* agc_step4,
    uint8_t* agc_step5,
    uint8_t* pll_bandwidth)
{
    uint8_t reg = read_reg(lora, REG_AGC_REF_LF);
    /* Sets the floor reference for all AGC thresholds:
       AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
       SNR = 8dB, fixed value */
    *agc_reference_level = (reg >> 0) & 0x3f;

    reg = read_reg(lora, REG_AGC_THRESH1_LF);
    /* Defines the 1st AGC Threshold */
    *agc_step1 = (reg >> 0) & 0x3f;

    reg = read_reg(lora, REG_AGC_THRESH2_LF);
    /* Defines the 2nd AGC Threshold */
    *agc_step2 = (reg >> 4) & 0xf;

    reg = read_reg(lora, REG_AGC_THRESH2_LF);
    /* Defines the 3rd AGC Threshold */
    *agc_step3 = (reg >> 0) & 0xf;

    reg = read_reg(lora, REG_AGC_THRESH3_LF);
    /* Defines the 4th AGC Threshold */
    *agc_step4 = (reg >> 4) & 0xf;

    reg = read_reg(lora, REG_AGC_THRESH3_LF);
    /* Defines the 5th AGC Threshold */
    *agc_step5 = (reg >> 0) & 0xf;

    reg = read_reg(lora, REG_PLL_LF);
    /* Controls the PLL bandwidth:
        00 - 75 kHz     10 - 225 kHz
        01 - 150 kHz    11 - 300 kHz */
    *pll_bandwidth = (reg >> 6) & 3;
}
#if (LORA_CHIP == SX1272)
static inline void set_pa_sx1272(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_PA_CONFIG);
    /* Selects PA output pin:
       0 RFIO pin. Output power is limited to 13 dBm.
       1 PA_BOOST pin. Output power is limited to 20 dBm */
    reg = setbits(reg, 7, 1, LORA_PA_SELECT);
    /* power amplifier max output power:
        Pout =  2 + OutputPower(3:0) on PA_BOOST.
        Pout = -1 + OutputPower(3:0) on RFIO. */
    reg = setbits(reg, 3, 4, OUTPUT_POWER);
    write_reg(lora, REG_PA_CONFIG, reg);

    reg = read_reg(lora, REG_PA_RAMP);
    /* LowPnTxPllOff:
        1 Low consumption PLL is used in receive and transmit mode
        0 Low consumption PLL in receive mode, low phase noise
        PLL in transmit mode. */
    reg = setbits(reg, 4, 1, SX127X_LOW_PN_TX_PLL_OFF);
    /* Rise/Fall time of ramp up/down in FSK
        0000 - 3.4 ms       0001 - 2 ms
        0010 - 1 ms         0011 - 500 us
        0100 - 250 us       0101 - 125 us
        0110 - 100 us       0111 - 62 us
        1000 - 50 us        1001 - 40 us
        1010 - 31 us        1011 - 25 us
        1100 - 20 us        1101 - 15 us
        1110 - 12 us        1111 - 10 us */
    reg = setbits(reg, 3, 4, SX127X_PA_RAMP);
    write_reg(lora, REG_PA_RAMP, reg);
}
#elif (LORA_CHIP == SX1276)
static inline void set_pa_sx1276(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_PA_CONFIG);
    /* Selects PA output pin
       0 - RFO pin. Output power is limited to +14 dBm.
       1 - PA_BOOST pin. Output power is limited to +20 dBm */
    reg = setbits(reg, 7, 1, LORA_PA_SELECT);
    /* Select max output power: Pmax=10.8+0.6*MaxPower [dBm] */
    reg = setbits(reg, 6, 3, MAX_POWER);
    /* Pout=Pmax-(15-OutputPower) if PaSelect = 0 (RFO pin)
       Pout=17  -(15-OutputPower) if PaSelect = 1 (PA_BOOST pin) */
    reg = setbits(reg, 3, 4, OUTPUT_POWER);
    write_reg(lora, REG_PA_CONFIG, reg);

    reg = read_reg(lora, REG_PA_RAMP);
    /* Rise/Fall time of ramp up/down in FSK
        0000 - 3.4 ms       0001 - 2 ms
        0010 - 1 ms         0011 - 500 us
        0100 - 250 us       0101 - 125 us
        0110 - 100 us       0111 - 62 us
        1000 - 50 us        1001 - 40 us
        1010 - 31 us        1011 - 25 us
        1100 - 20 us        1101 - 15 us
        1110 - 12 us        1111 - 10 us */
    reg = setbits(reg, 3, 4, SX127X_PA_RAMP);
    write_reg(lora, REG_PA_RAMP, reg);
}
#endif
static inline uint8_t get_pa_select(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_PA_CONFIG);
    /* 0 RFIO pin. Output power is limited to 13 dBm.
       1 PA_BOOST pin. Output power is limited to 20 dBm */
    return (reg >> 7) & 1;
}
static inline int8_t get_tx_output_power(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_PA_CONFIG);
    /* power amplifier max output power:
        Pout = 2 + OutputPower(3:0) on PA_BOOST.
        Pout = -1 + OutputPower(3:0) on RFIO. */
    return (reg >> 0) & 0xf;
}
static inline uint8_t get_max_power_sx1276(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_PA_CONFIG);
    /* Select max output power: Pmax=10.8+0.6*MaxPower [dBm] */
    return (reg >> 4) & 7;
}
static inline uint8_t get_pa_dac(LORA* lora);
static inline int8_t get_tx_output_power_dbm(LORA* lora)
{
    int8_t output_power, pout_dbm, pmax_dbm;
    uint8_t pa_dac;

    output_power = get_tx_output_power(lora);
    pa_dac       = get_pa_dac(lora);

    if (LORA_CHIP == SX1272)
    {
        /* [pa_select] Selects PA output pin:
                0 RFIO pin. Output power is limited to 13 dBm.
                1 PA_BOOST pin. Output power is limited to 20 dBm */
        /* power amplifier max output power:
            Pout =  2 + OutputPower(3:0) on PA_BOOST.
            Pout = -1 + OutputPower(3:0) on RFIO. */
        if (LORA_PA_SELECT == 1)
        {
            /* The SX1272/73 has a high power +20 dBm capability on PA_BOOST pin
               (see 5.4.3. High Power +20 dBm Operation) */
            /* Enables the +20 dBm option on PA_BOOST pin
               0x04 - Default value
               0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
            //NOTE: see Pout Formula in Table 31 Power Amplifier Mode Selection Truth Table
            if      (pa_dac == 7)   pout_dbm = 5 + output_power;
            else if (pa_dac == 4)   pout_dbm = 2 + output_power;
            else
            {
                pout_dbm = -128;
                error (ERROR_INVALID_PARAMS);
            }
        }
        else
        {
            //NOTE: see Pout Formula in Table 31 Power Amplifier Mode Selection Truth Table
            pout_dbm = -1 + output_power;
        }
    }
    else if (LORA_CHIP == SX1276)
    {
        /* [pa_select] Selects PA output pin:
                0 RFIO pin. Output power is limited to 13 dBm.
                1 PA_BOOST pin. Output power is limited to 20 dBm */
        if (LORA_PA_SELECT == 1)
        {
            /* The SX1276/77/78/79 have a high power +20 dBm capability on PA_BOOST pin
               see (5.4.3. High Power +20 dBm Operation) */
            /* Enables the +20 dBm option on PA_BOOST pin
               0x04 - Default value
               0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
            if      (pa_dac == 7)    pout_dbm = 20 - (15 - output_power);
            /* Pout=17-(15-OutputPower) [dBm] */
            else if (pa_dac == 4)    pout_dbm = 17 - (15 - output_power);
            else
            {
                pout_dbm = -128;
                error (ERROR_INVALID_PARAMS);
            }
        }
        else
        {
            /*  Pout=Pmax-(15-OutputPower)
                Pmax=10.8+0.6*MaxPower [dBm] */
            pmax_dbm = (108 + 6 * sx1276_get_max_power(lora)) / 10;
            pout_dbm = pmax_dbm - (15-output_power);
        }
    }
    else
    {
        pout_dbm = -128;
        error (ERROR_INVALID_PARAMS);
    }
    return pout_dbm;
}
static inline uint8_t get_low_pn_tx_pll_off(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_PA_RAMP);
    /* LowPnTxPllOff:
        1 Low consumption PLL is used in receive and transmit mode
        0 Low consumption PLL in receive mode, low phase noise
        PLL in transmit mode. */
    return (reg >> 4) & 1;
}
static inline uint8_t get_pa_ramp(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_PA_RAMP);
    /* Rise/Fall time of ramp up/down in FSK
        0000 - 3.4 ms       0001 - 2 ms
        0010 - 1 ms         0011 - 500 us
        0100 - 250 us       0101 - 125 us
        0110 - 100 us       0111 - 62 us
        1000 - 50 us        1001 - 40 us
        1010 - 31 us        1011 - 25 us
        1100 - 20 us        1101 - 15 us
        1110 - 12 us        1111 - 10 us */
    return (reg >> 0) & 0xf;
}
/* 2.5.1. Power Consumption -- Table 6 - Power Consumption Specification
   IDDT -- Supply current in Transmit mode with impedance matching
   Conditions                  Typ   Unit
   RFOP = +20 dBm on PA_BOOST  125   mA
   RFOP = +17 dBm on PA_BOOST   90   mA
   RFOP = +13 dBm on RFO pin    28   mA
   RFOP = + 7 dBm on RFO pin    18   mA */
static inline uint8_t get_iddt_ma_sx1272(int8_t tx_output_power_dbm)
{
    if (tx_output_power_dbm <= 7)
    {
        //given value 18  (Table 6)
        return 18;
    }

    switch (tx_output_power_dbm)
    {
    case 8:
        return 19;
    case 9:
        return 20;
    case 10:
        return 22;
    case 11:
        return 24;
    case 12:
        return 26;
    case 13:
        //given value 28  (Table 6)
        return 28;
    case 14:
        return 37;
    case 15:
        return 52;
    case 16:
        return 71;
    case 17:
        //given value 90  (Table 6)
        return 90;
    case 18:
        return 105;
    case 19:
        return 117;
    case 20:
        //given value 125 (Table 6)
        return 125;
    default:
        error (ERROR_NOT_SUPPORTED);
        return 0;
    }
}
/* 2.5.1. Power Consumption -- Table 6 - Power Consumption Specification
   IDDT -- Supply current in Transmit mode with impedance matching
   Conditions                  Typ   Unit
   RFOP = +20 dBm on PA_BOOST  120   mA
   RFOP = +17 dBm on PA_BOOST   87   mA
   RFOP = +13 dBm on RFO pin    29   mA
   RFOP = + 7 dBm on RFO pin    20   mA */
static inline uint8_t get_iddt_ma_sx1276(int8_t tx_output_power_dbm)
{
    if (tx_output_power_dbm <= 7)
    {
        //given value 20  (Table 6)
        return 20;
    }

    switch (tx_output_power_dbm)
    {
    case 8:
        return 20;
    case  9:
        return 21;
    case 10:
        return 23;
    case 11:
        return 25;
    case 12:
        return 27;
    case 13:
        //given value 29  (Table 6)
        return 29;
    case 14:
        return 37;
    case 15:
        return 51;
    case 16:
        return 69;
    case 17:
        //given value 87  (Table 6)
        return 87;
    case 18:
        return 101;
    case 19:
        return 112;
    case 20:
        //given value 120 (Table 6)
        return 120;
    default:
        error (ERROR_NOT_SUPPORTED);
        return 0;
    }
}
static inline uint8_t get_iddt_ma(LORA* lora, int8_t tx_output_power_dbm)
{
    switch (LORA_CHIP)
    {
    case SX1272:
        return get_iddt_ma_sx1272(tx_output_power_dbm);
    case SX1276:
        return get_iddt_ma_sx1276(tx_output_power_dbm);
    default:
        error (ERROR_NOT_SUPPORTED);
        return 0;
    }
}
/* Trimming of OCP current (Table 35)
   CurrentOcpTrim  Imax            Imax Formula
   0  to 15         45 to 120 mA    45 +  5*OcpTrim [mA]
   16 to 27        130 to 240 mA   -30 + 10*OcpTrim [mA]
   27+             240 mA          240 [mA]
   Default: OcpTrim = 0x0B (Imax = 100 mA) */
static inline uint8_t get_ocp_trim_by_imax(LORA* lora, uint8_t imax_ma)
{
    if      (imax_ma >= 240) return 27;
    else if (imax_ma >= 130) return (imax_ma + 30) / 10;
    //note: range 121..129 is not covered by Table 35
    else if (imax_ma >= 121) return (imax_ma - 45) / 5;
    else if (imax_ma >=  45) return (imax_ma - 45) / 5;
    else                     return 0;
}
static inline void set_ocp(LORA* lora)
{
    uint8_t reg, ocp_trim, iddt_ma;
    int8_t tx_output_power_dbm;
    reg = read_reg(lora, REG_OCP);
    /* Enables overload current protection (OCP) for PA:
        0 - OCP disabled
        1 - OCP enabled */
    reg = setbits(reg, 5, 1, SX127X_OCP_ON);
    //Notes:
    // - The Over Current Protection limit should be adapted to
    //   the actual power level, in RegOcp
    if (SX127X_OCP_ON)
    {
        /* Trimming of OCP current (Table 35)
           CurrentOcpTrim  Imax            Imax Formula
           0  to 15         45 to 120 mA    45 +  5*OcpTrim [mA]
           16 to 27        130 to 240 mA   -30 + 10*OcpTrim [mA]
           27+             240 mA          240 [mA]
           Default: OcpTrim = 0x0B (Imax = 100 mA) */
        tx_output_power_dbm = get_tx_output_power_dbm(lora);
        iddt_ma             = get_iddt_ma(lora, tx_output_power_dbm);
        ocp_trim            = get_ocp_trim_by_imax(lora, iddt_ma);
        reg = setbits(reg, 4, 5, ocp_trim);
    }
    write_reg(lora, REG_OCP, reg);
}
static inline uint8_t get_ocp_on(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_OCP);
    /* Enables overload current protection (OCP) for PA:
        0 - OCP disabled
        1 - OCP enabled */
    return (reg >> 5) & 1;
}
static inline uint8_t get_ocp_trim(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_OCP);
    /* Trimming of OCP current (Table 35)
       CurrentOcpTrim  Imax            Imax Formula
       0  to 15         45 to 120 mA    45 +  5*OcpTrim [mA]
       16 to 27        130 to 240 mA   -30 + 10*OcpTrim [mA]
       27+             240 mA          240 [mA]
       Default: OcpTrim = 0x0B (Imax = 100 mA) */
    return (reg >> 0) & 0x1f;
}
static inline void set_lna(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_LNA);
    /* LNA gain setting:
        000 - not used
        001 - G1 = maximum gain
        010 - G2
        011 - G3
        100 - G4
        101 - G5
        110 - G6 = minimum gain
        111 - not used */
    reg = setbits(reg, 7, 3, SX127X_LNA_GAIN);
    /*  LnaBoost:
        00 - Default LNA current
        11 - Boost on, 150% LNA current. */
    reg = setbits(reg, 1, 2, SX127X_LNA_BOOST);
    write_reg(lora, REG_LNA, reg);
}
static inline uint8_t get_lna_gain(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_LNA);
    /* LNA gain setting:
        000 - not used
        001 - G1 = maximum gain
        010 - G2
        011 - G3
        100 - G4
        101 - G5
        110 - G6 = minimum gain
        111 - not used */
    return (reg >> 7) & 7;
}
static inline uint8_t get_lna_boost(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_LNA);
    /*  LnaBoost:
        00 - Default LNA current
        11 - Boost on, 150% LNA current. */
    return (reg >> 0) & 3;
}
static inline void set_fifo_address_pointers(LORA* lora)
{
    /* SPI interface address pointer in FIFO data buffer. */
    write_reg(lora, REG_FIFO_ADDR_PTR, FIFO_ADDR_PTR);
    /* write base address in FIFO data buffer for TX modulator */
    write_reg(lora, REG_FIFO_TX_BASE_ADDR, FIFO_TX_BASE_ADDR);
    /* read base address in FIFO data buffer for RX demodulator */
    write_reg(lora, REG_FIFO_RX_BASE_ADDR, FIFO_RX_BASE_ADDR);
}
static inline void get_fifo_address_pointers(LORA* lora,
    uint8_t* fifo_addr_ptr, uint8_t* fifo_tx_base_addr,
    uint8_t* fifo_rx_base_addr, uint8_t* fifo_rx_current_addr)
{
    /* SPI interface address pointer in FIFO data buffer. */
    *fifo_addr_ptr = read_reg(lora, REG_FIFO_ADDR_PTR);
    /* write base address in FIFO data buffer for TX modulator */
    *fifo_tx_base_addr = read_reg(lora, REG_FIFO_TX_BASE_ADDR);
    /* read base address in FIFO data buffer for RX demodulator */
    *fifo_rx_base_addr = read_reg(lora, REG_FIFO_RX_BASE_ADDR);
    /* Start address (in data buffer) of last packet received */
    *fifo_rx_current_addr = read_reg(lora, REG_FIFO_RX_CURRENT_ADDR);
}
static inline void get_fifo_rx_current_addr(LORA* lora, uint8_t* fifo_rx_current_addr)
{
    /* Start address (in data buffer) of last packet received */
    *fifo_rx_current_addr = read_reg(lora, REG_FIFO_RX_CURRENT_ADDR);
}
static inline void set_fifo_addr_ptr(LORA* lora, uint8_t fifo_addr_ptr)
{
    /* SPI interface address pointer in FIFO data buffer. */
    write_reg(lora, REG_FIFO_ADDR_PTR, fifo_addr_ptr);
}
static inline void set_reg_irq_flags_mask(LORA* lora, irq_t irq, uint8_t value)
{
    uint8_t reg;
    reg = read_reg(lora, REG_IRQ_FLAGS_MASK);
    /* XXX: setting this bit masks the corresponding IRQ in RegIrqFlags */
    reg = setbits(reg, (int)irq, 1, value);
    write_reg(lora, REG_IRQ_FLAGS_MASK, reg);
}
static inline void set_reg_irq_flags_mask_all(LORA* lora)
{
    write_reg(lora, REG_IRQ_FLAGS_MASK, 0x00);
}
static inline uint8_t get_reg_irq_flags_mask(LORA* lora)
{
    return read_reg(lora, REG_IRQ_FLAGS_MASK);
}
static inline void set_reg_irq_flags_clear(LORA* lora, irq_t irq)
{
    /* XXX interrupt: writing a 1 clears the IRQ */
    uint8_t reg;
    reg = read_reg(lora, REG_IRQ_FLAGS_MASK);
    reg = setbits(reg, (int)irq, 1, 1);
    write_reg(lora, REG_IRQ_FLAGS, reg);
}
static inline void set_reg_irq_flags_clear_all(LORA* lora)
{
    /* XXX interrupt: writing a 1 clears the IRQ */
    write_reg(lora, REG_IRQ_FLAGS, 0xff);
}
static inline uint8_t get_reg_irq_flags(LORA* lora, irq_t irq, uint8_t reg)
{
    return (reg >> (int)irq) & 1;
}
static inline uint8_t get_reg_irq_flags_all(LORA* lora)
{
    return read_reg(lora, REG_IRQ_FLAGS);
}
static inline uint8_t get_latest_pkt_rcvd_payload_size(LORA* lora)
{
    return read_reg(lora, REG_RX_NB_BYTES);
}
static inline uint16_t get_valid_headers_received_num(LORA* lora)
{
    uint16_t value = 0;
    value |= read_reg(lora, REG_RX_HEADER_CNT_VALUE_LSB);
    value |= read_reg(lora, REG_RX_HEADER_CNT_VALUE_MSB) << 8;
    return value;
}
static inline uint16_t get_valid_packets_received_num(LORA* lora)
{
    uint16_t value = 0;
    value |= read_reg(lora, REG_RX_PACKET_CNT_VALUE_LSB);
    value |= read_reg(lora, REG_RX_PACKET_CNT_VALUE_MSB) << 8;
    return value;
}
static inline uint8_t get_rx_coding_rate(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_STAT);
    /* Coding rate of last header received */
    return (reg >> 7) & 7;
}
static inline uint8_t get_modem_status(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_STAT);
    /* Coding rate of last header received */
    return (reg >> 0) & 0x1f;
}
static inline int8_t get_snr_last_pkt_rcvd_db(LORA* lora)
{
    int8_t snr, reg;
    reg = read_reg(lora, REG_PKT_SNR_VALUE);
    /* Estimation of SNR on last packet received. In two's compliment
        format multiplied by 4.*/
    snr = (int8_t)reg / 4;
    return snr;
}
static inline int8_t get_snr_on_last_packet_received_tc(LORA* lora)
{
    return (int8_t)read_reg(lora, REG_PKT_SNR_VALUE);
}
static inline int16_t get_rssi_last_pkt_rcvd_dbm(LORA* lora)
{
    int16_t rssi_dbm; int8_t snr_db, snr_tc; uint8_t rssi_pkt;
    rssi_dbm = 0;  snr_db = 0;  snr_tc = 0;  rssi_pkt = 0;
    rssi_pkt = read_reg(lora, REG_PKT_RSSI_VALUE);
#if (LORA_DEBUG)
    printf("[sx127x] [info] rssi_pkt %u\n", rssi_pkt);
#endif
    snr_db = get_snr_last_pkt_rcvd_db(lora);
    if (LORA_CHIP == SX1272)
    {
        /* RSSI of the latest packet received (dBm)
               RSSI[dBm] = - 139 + PacketRssi (when SNR >= 0)
               or
               RSSI[dBm] = - 139 + PacketRssi + PacketSnr *0.25 (when SNR < 0) */
        snr_tc = get_snr_on_last_packet_received_tc(lora);
#if (LORA_DEBUG)
        printf("[sx127x] [info] snr_tc %d rssi_pkt %d\n", snr_tc, rssi_pkt);
#endif
        if (snr_db >= 0)
            rssi_dbm = -139 + (int16_t)rssi_pkt;
        else
            rssi_dbm = -139 + (int16_t)rssi_pkt + (int16_t)(snr_tc / 4);
    }
    else if (LORA_CHIP == SX1276)
    {
        /* RSSI of the latest packet received (dBm):
               RSSI[dBm] = -157 + Rssi (using HF output port, SNR >= 0)
               or
               RSSI[dBm] = -164 + Rssi (using LF output port, SNR >= 0)
               (see section 5.5.5 for details) */
        if (snr_db >= 0) {
            if (!get_low_frequency_mode_on(lora))
                rssi_dbm = -157 + (int16_t)rssi_pkt;
            else
                rssi_dbm = -164 + (int16_t)rssi_pkt;
        }
    }
    return rssi_dbm;
}
static inline int16_t get_rssi_cur_dbm(LORA* lora)
{
    int16_t rssi_dbm; uint8_t rssi;
    rssi_dbm = 0; rssi = 0;
    rssi = read_reg(lora, REG_RSSI_VALUE);
#if (LORA_DEBUG)
    printf("[sx127x] [info] rssi %u\n", rssi);
#endif
    if (LORA_CHIP == SX1272)
    {
        /* Current RSSI value (dBm)
            RSSI[dBm] = - 139 + Rssi */
        rssi_dbm = -139 + (int16_t)rssi;
    }
    else if (LORA_CHIP == SX1276)
    {
        /* Current RSSI value (dBm)
            RSSI[dBm] = -157 + Rssi (using HF output port)
            or
            RSSI[dBm] = -164 + Rssi (using LF output port)
            (see section 5.5.5 for details) */
        if (!get_low_frequency_mode_on(lora))
            rssi_dbm = -157 + (int16_t)rssi;
        else
            rssi_dbm = -164 + (int16_t)rssi;
    }
    return rssi_dbm;
}
static inline uint8_t get_pll_failed_to_lock_while_tx_rx_cad_operation(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_HOP_CHANNEL);
    /* PLL failed to lock while attempting a TX/RX/CAD operation
        1 - PLL did not lock
        0 - PLL did lock */
    return (reg >> 7) & 1;
}
static inline bool crc_is_present_in_rcvd_pkt_header_ext(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_HOP_CHANNEL);
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    return (reg >> 6) & 1;
}
//note: returns predefined value to make crc flag independent of tx flag (see get_pkt_time_on_air_ms)
static inline bool crc_is_present_in_rcvd_pkt_header(LORA* lora)
{
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    return LORA_RX_PAYLOAD_CRC_ON;
}
static inline uint8_t get_frequency_hopping_channel_current_value(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_HOP_CHANNEL);
    /* Current value of frequency hopping channel in use. */
    return (reg >> 0) & 0x3f;
}
static inline void set_modem_config(LORA* lora, uint8_t low_data_rate_optimize)
{
    uint8_t reg;
    if (LORA_CHIP == SX1272)
    {
        reg = read_reg(lora, REG_MODEM_CONFIG1);
        /* Signal bandwidth:
            00 - 125 kHz
            01 - 250 kHz
            10 - 500 kHz
            11 - reserved */
        reg = setbits(reg, 7, 2, LORA_SIGNAL_BANDWIDTH);
        /*  Error coding rate
            001 - 4/5
            010 - 4/6
            011 - 4/7
            100 - 4/8
            All other values - reserved
            In implicit header mode should be set on receiver to determine
            expected coding rate. See Section 4.1.1.3. */
        reg = setbits(reg, 5, 3, LORA_CODING_RATE);
        /* ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
        reg = setbits(reg, 2, 1, LORA_IMPLICIT_HEADER_MODE_ON);
        /* Enable CRC generation and check on payload:
            0 - CRC disable
            1 - CRC enable
            If CRC is needed, RxPayloadCrcOn should be set:
            - in Implicit header mode: on Tx and Rx side
            - in Explicit header mode: on the Tx side alone (recovered from
            the header in Rx side) */
        reg = setbits(reg, 1, 1, LORA_RX_PAYLOAD_CRC_ON);
        /* LowDataRateOptimize:
            0 - Disabled
            1 - Enabled; mandated for SF11 and SF12 with BW = 125 kHz */
        reg = setbits(reg, 0, 1, low_data_rate_optimize);
        write_reg(lora, REG_MODEM_CONFIG1, reg);

        reg = read_reg(lora, REG_MODEM_CONFIG2);
        /* SF rate (expressed as a base-2 logarithm)
             6 -   64 chips / symbol
             7 -  128 chips / symbol
             8 -  256 chips / symbol
             9 -  512 chips / symbol
            10 - 1024 chips / symbol
            11 - 2048 chips / symbol
            12 - 4096 chips / symbol
            other values reserved. */
        reg = setbits(reg, 7, 4, LORA_SPREADING_FACTOR);
        /* TxContinuousMode:
            0 - normal mode, a single packet is sent
            1 - continuous mode, send multiple packets across the FIFO
                (used for spectral analysis) */
        reg = setbits(reg, 3, 1, SX127X_TX_CONTINUOUS_MODE);
        /* AgcAutoOn:
            0 - LNA gain set by register LnaGain
            1 - LNA gain set by the internal AGC loop */
        reg = setbits(reg, 2, 1, SX127X_AGC_AUTO_ON);
        write_reg(lora, REG_MODEM_CONFIG2, reg);
    }
    else if (LORA_CHIP == SX1276)
    {
        reg = read_reg(lora, REG_MODEM_CONFIG1);
        /* Signal bandwidth:
            0000 - 7.8 kHz            0001 - 10.4 kHz
            0010 - 15.6 kHz           0011 - 20.8kHz
            0100 - 31.25 kHz          0101 - 41.7 kHz
            0110 - 62.5 kHz           0111 - 125 kHz
            1000 - 250 kHz            1001 - 500 kHz
            other values - reserved
            In the lower band (169MHz), signal bandwidths 8&9 are not supported) */
        reg = setbits(reg, 7, 4, LORA_SIGNAL_BANDWIDTH);
        /*  Error coding rate
            001 - 4/5
            010 - 4/6
            011 - 4/7
            100 - 4/8
            All other values - reserved
            In implicit header mode should be set on receiver to determine
            expected coding rate. See Section 4.1.1.3. */
        reg = setbits(reg, 3, 3, LORA_CODING_RATE);
        /* ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
        reg = setbits(reg, 0, 1, LORA_IMPLICIT_HEADER_MODE_ON);
        write_reg(lora, REG_MODEM_CONFIG1, reg);

        reg = read_reg(lora, REG_MODEM_CONFIG2);
        /* SF rate (expressed as a base-2 logarithm)
             6 -   64 chips / symbol
             7 -  128 chips / symbol
             8 -  256 chips / symbol
             9 -  512 chips / symbol
            10 - 1024 chips / symbol
            11 - 2048 chips / symbol
            12 - 4096 chips / symbol
            other values reserved. */
        reg = setbits(reg, 7, 4, LORA_SPREADING_FACTOR);
        /* TxContinuousMode:
            0 - normal mode, a single packet is sent
            1 - continuous mode, send multiple packets across the FIFO
                (used for spectral analysis) */
        reg = setbits(reg, 3, 1, SX127X_TX_CONTINUOUS_MODE);
        /* Enable CRC generation and check on payload:
            0 - CRC disable
            1 - CRC enable
            If CRC is needed, RxPayloadCrcOn should be set:
            - in Implicit header mode: on Tx and Rx side
            - in Explicit header mode: on the Tx side alone (recovered from
            the header in Rx side) */
        reg = setbits(reg, 2, 1, LORA_RX_PAYLOAD_CRC_ON);
        write_reg(lora, REG_MODEM_CONFIG2, reg);

        reg = read_reg(lora, REG_MODEM_CONFIG3);
        /* LowDataRateOptimize:
            0 - Disabled
            1 - Enabled; mandated for when the symbol length exceeds 16ms */
        reg = setbits(reg, 3, 1, low_data_rate_optimize);
        /* AgcAutoOn:
            0 - LNA gain set by register LnaGain
            1 - LNA gain set by the internal AGC loop */
        reg = setbits(reg, 2, 1, SX127X_AGC_AUTO_ON);
        write_reg(lora, REG_MODEM_CONFIG3, reg);
    }
}
static inline uint8_t get_modem_config_signal_bandwidth(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG1);
    if (LORA_CHIP == SX1272)
    {
        /* Signal bandwidth:
            00 - 125 kHz
            01 - 250 kHz
            10 - 500 kHz
            11 - reserved */
        return (reg >> 6) & 3;
    }
    else if (LORA_CHIP == SX1276)
    {
        /* Signal bandwidth:
            0000 - 7.8 kHz            0001 - 10.4 kHz
            0010 - 15.6 kHz           0011 - 20.8kHz
            0100 - 31.25 kHz          0101 - 41.7 kHz
            0110 - 62.5 kHz           0111 - 125 kHz
            1000 - 250 kHz            1001 - 500 kHz
            other values - reserved
            In the lower band (169MHz), signal bandwidths 8&9 are not supported) */
        return (reg >> 4) & 0xf;
    }
    return 0;
}
static inline uint32_t get_modem_config_signal_bandwidth_hz_ext(LORA* lora, uint8_t bw)
{
    if (LORA_CHIP == SX1272)
    {
        /* Signal bandwidth:
            00 - 125 kHz            01 - 250 kHz
            10 - 500 kHz            11 - reserved */
        switch (bw)
        {
        case 0:
            return 125000;
        case 1:
            return 250000;
        case 2:
            return 500000;
        case 3:
            return 0;
        default:
            error (ERROR_NOT_SUPPORTED);
            return 0;
        }
    }
    else if (LORA_CHIP == SX1276)
    {
        /* Signal bandwidth:
        0000 - 7.8  kHz        0001 - 10.4 kHz
        0010 - 15.6 kHz        0011 - 20.8 kHz
        0100 - 31.25 kHz       0101 - 41.7 kHz
        0110 - 62.5 kHz        0111 - 125  kHz
        1000 - 250  kHz        1001 - 500  kHz
        other values - reserved
        In the lower band (169MHz), signal bandwidths 8&9 are not
        supported) */
        switch (bw)
        {
        case 0:
            return 7800;
        case 1:
            return 10400;
        case 2:
            return 15600;
        case 3:
            return 20800;
        case 4:
            return 31250;
        case 5:
            return 41700;
        case 6:
            return 62500;
        case 7:
            return 125000;
        case 8:
            return 250000;
        case 9:
            return 500000;
        default:
            error (ERROR_NOT_SUPPORTED);
            return 0;
        }
    }
}
static inline uint32_t get_modem_config_signal_bandwidth_hz(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG1), bw;

    switch(LORA_CHIP)
    {
    case SX1272:
        bw = (reg >> 6) & 0x3;
        break;
    case SX1276:
        bw = (reg >> 4) & 0xf;
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        return 0;
    }
    return get_modem_config_signal_bandwidth_hz_ext(lora, bw);
}
static inline uint8_t get_modem_config_coding_rate(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG1);
    /*  Error coding rate
        001 - 4/5
        010 - 4/6
        011 - 4/7
        100 - 4/8
        All other values - reserved
        In implicit header mode should be set on receiver to determine
        expected coding rate. See Section 4.1.1.3. */
    if (LORA_CHIP == SX1272)
        return (reg >> 3) & 7;
    else if (LORA_CHIP == SX1276)
        return (reg >> 1) & 7;
    return 0;
}
static inline uint8_t get_modem_config_implicit_header_mode_on(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG1);
    /* ImplicitHeaderModeOn:
        0 - Explicit Header mode
        1 - Implicit Header mode */
    if (LORA_CHIP == SX1272)
        return (reg >> 2) & 1;
    else if (LORA_CHIP == SX1276)
        return (reg >> 0) & 1;
    return 0;
}
static inline uint8_t get_modem_config_rx_payload_crc_on(LORA* lora)
{
    uint8_t reg;
    /* Enable CRC generation and check on payload:
        0 - CRC disable
        1 - CRC enable
        If CRC is needed, RxPayloadCrcOn should be set:
        - in Implicit header mode: on Tx and Rx side
        - in Explicit header mode: on the Tx side alone (recovered from
        the header in Rx side) */
    if (LORA_CHIP == SX1272)
    {
        reg = read_reg(lora, REG_MODEM_CONFIG1);
        return (reg >> 1) & 1;
    }
    else if (LORA_CHIP == SX1276)
    {
        reg = read_reg(lora, REG_MODEM_CONFIG2);
        return (reg >> 2) & 1;
    }
    return 0;
}
static inline uint8_t get_modem_config_low_data_rate_optimize(LORA* lora)
{
    uint8_t reg;
    /* LowDataRateOptimize:
        0 - Disabled
        1 - Enabled; mandated for SF11 and SF12 with BW = 125 kHz */
    if (LORA_CHIP == SX1272)
    {
        reg = read_reg(lora, REG_MODEM_CONFIG1);
        return (reg >> 0) & 1;
    }
    else if (LORA_CHIP == SX1276)
    {
        reg = read_reg(lora, REG_MODEM_CONFIG3);
        return (reg >> 3) & 1;
    }
    return 0;
}
static inline uint8_t get_modem_config_spreading_factor(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG2);
    /* SF rate (expressed as a base-2 logarithm)
         6 -   64 chips / symbol
         7 -  128 chips / symbol
         8 -  256 chips / symbol
         9 -  512 chips / symbol
        10 - 1024 chips / symbol
        11 - 2048 chips / symbol
        12 - 4096 chips / symbol
        other values reserved. */
    return (reg >> 4) & 7;
}
static inline uint8_t get_modem_config_tx_continuous_mode(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG2);
    /* TxContinuousMode:
        0 - normal mode, a single packet is sent
        1 - continuous mode, send multiple packets across the FIFO
            (used for spectral analysis) */
    return (reg >> 3) & 1;
}
static inline uint8_t get_modem_config_agc_auto_on(LORA* lora)
{
    uint8_t reg;
    /* AgcAutoOn:
        0 - LNA gain set by register LnaGain
        1 - LNA gain set by the internal AGC loop */
    if (LORA_CHIP == SX1272)
        reg = read_reg(lora, REG_MODEM_CONFIG2);
    else if (LORA_CHIP == SX1276)
        reg = read_reg(lora, REG_MODEM_CONFIG3);
    else
        reg = 0;
    return (reg >> 2) & 1;
}
static inline void set_ppm_correction(LORA* lora)
{
    /* Data rate offset value, used in conjunction with AFC */
    write_reg(lora, REG_PPM_CORRECTION, SX127X_PPM_CORRECTION);
}
static inline uint8_t get_ppm_correction(LORA* lora)
{
    /* Data rate offset value, used in conjunction with AFC */
    return read_reg(lora, REG_PPM_CORRECTION);
}
static inline void set_rx_timeout(LORA* lora, uint16_t timeout_symb)
{
    uint8_t reg;
    reg = read_reg(lora, REG_MODEM_CONFIG2);
    /* RX Time-Out MSB */
    reg = setbits(reg, 1, 2, (timeout_symb >> 8));
    write_reg(lora, REG_MODEM_CONFIG2, reg);

    reg = read_reg(lora, REG_SYMB_TIMEOUT_LSB);
    /* RX Time-Out LSB
       RX operation time-out value expressed as number of symbols:
       TimeOut = SymbTimeout * Ts */
    reg = setbits(reg, 7, 8, (timeout_symb >> 0));
    write_reg(lora, REG_SYMB_TIMEOUT_LSB, reg);
}
static inline uint16_t get_rx_timeout_symb(LORA* lora)
{
    uint16_t value = 0;
    /* RX Time-Out LSB */
    value |= read_reg(lora, REG_SYMB_TIMEOUT_LSB);
    /* RX Time-Out MSB */
    value |= (read_reg(lora, REG_MODEM_CONFIG2) & 3) << 8;
    return value;
}
//NOTE: depends on get_modem_config_signal_bandwidth_hz
uint32_t rx_timeout_symb_to_ms(LORA* lora, uint32_t timeout_symb)
{
    uint32_t bw, sf, r_s, t_s, timeout_ms;
    bw = get_modem_config_signal_bandwidth_hz(lora);
    sf = LORA_SPREADING_FACTOR;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * DOMAIN_US) / r_s;
    timeout_ms = (timeout_symb * t_s) / 1000;
    return timeout_ms;
}
//NOTE: depends on get_modem_config_signal_bandwidth_hz
uint32_t rx_timeout_ms_to_symb(LORA* lora, uint32_t timeout_ms)
{
    uint32_t bw, sf, r_s, t_s, timeout_symb;
    bw = get_modem_config_signal_bandwidth_hz(lora);
    sf = LORA_SPREADING_FACTOR;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * DOMAIN_US) / r_s;
    timeout_symb = (timeout_ms * 1000) / t_s;
    return timeout_symb;
}
//NOTE: unused (returns same value as lora->rx_timeout_ms)
uint32_t get_rx_timeout_ms_single_reception(LORA* lora)
{
    uint32_t timeout_symb, timeout_ms;
    timeout_symb = get_rx_timeout_symb(lora);
    timeout_ms = rx_timeout_symb_to_ms(lora, timeout_symb);
    return timeout_ms;
}
static inline void set_preamble_length(LORA* lora)
{
    /* Preamble length MSB, = PreambleLength + 4.25 Symbols
        See Section 4.1.1.6 for more details. */
    write_reg(lora, REG_PREAMBLE_MSB, (LORA_PREAMBLE_LEN >> 8) & 0xff);
    /* Preamble Length LSB */
    write_reg(lora, REG_PREAMBLE_LSB, LORA_PREAMBLE_LEN & 0xff);
}
static inline uint16_t get_preamble_length(LORA* lora)
{
    uint16_t value = 0;
    /* Preamble Length LSB */
    value |= read_reg(lora, REG_PREAMBLE_LSB);
    /* Preamble length MSB, = PreambleLength + 4.25 Symbols
        See Section 4.1.1.6 for more details. */
    value |= read_reg(lora, REG_PREAMBLE_MSB) << 8;
    return value;
}
static inline int set_payload_length(LORA* lora, uint8_t payload_length)
{
    /* A 0 value is not permitted */
    if (payload_length == 0)
        return -1;
    write_reg(lora, REG_PAYLOAD_LENGTH, payload_length);
    return 0;
}
static inline uint8_t get_payload_length_ext(LORA* lora)
{
    /* Payload length in bytes. The register needs to be set in implicit
       header mode for the expected packet length. A 0 value is not
       permitted */
    return read_reg(lora, REG_PAYLOAD_LENGTH);
}
static inline uint8_t get_payload_length(LORA* lora)
{
    uint8_t payload_length;

    if (lora->tx || LORA_IMPLICIT_HEADER_MODE_ON)
    {
        payload_length = lora->io->data_size;
    }
    else if (!LORA_IMPLICIT_HEADER_MODE_ON)
    {
        payload_length = get_latest_pkt_rcvd_payload_size(lora);
    }
    return payload_length;
}
static inline void set_max_payload_length(LORA* lora)
{
    /* Maximum payload length; if header payload length exceeds
        value a header CRC error is generated. Allows filtering of packet
        with a bad size. */
    write_reg(lora, REG_MAX_PAYLOAD_LENGTH, LORA_MAX_PAYLOAD_LEN);
}
static inline uint8_t get_max_payload_length(LORA* lora)
{
    /* Maximum payload length; if header payload length exceeds
        value a header CRC error is generated. Allows filtering of packet
        with a bad size. */
    return read_reg(lora, REG_MAX_PAYLOAD_LENGTH);
}
static inline void set_symbol_periods_between_frequency_hops(LORA* lora)
{
    /* Symbol periods between frequency hops. (0 = disabled). 1st hop
        always happen after the 1st header symbol */
    write_reg(lora, REG_HOP_PERIOD, 0);
}
static inline uint8_t get_symbol_periods_between_frequency_hops(LORA* lora)
{
    /* Symbol periods between frequency hops. (0 = disabled). 1st hop
        always happen after the 1st header symbol */
    return read_reg(lora, REG_HOP_PERIOD);
}
static inline uint8_t get_rx_databuffer_pointer_current_value(LORA* lora)
{
    /* Current value of RX databuffer pointer (address of last byte
        written by Lora receiver) */
    return read_reg(lora, REG_FIFO_RX_BYTE_ADDR);
}
static inline int32_t get_rf_freq_error_hz(LORA* lora)
{
    /* NOTE: Two terms are being used:
             - "F_xtal - crystal frequency"
             - "F_xosc - crystal oscillator frequency (32 MHz)"
       Question: F_xtal == F_xosc??
       Assume F_xtal == F_xosc (32 MHz) (see Table 42 Crystal Specification) */
    const uint32_t f_xtal = 32000000;
    uint32_t f_err_tc, f_err, bw_hz;   int32_t f_err_i;
    bool is_negative;
    /* The contents of which are a signed 20 bit two's compliment word */
    /* LSB of RF Frequency Error */
    f_err_tc = 0; f_err = 0; bw_hz = 0;
    f_err_tc |= read_reg(lora, REG_FEI_LSB);
    /* Middle byte of RF Frequency Error */
    f_err_tc |= read_reg(lora, REG_FEI_MID) << 8;
    /* MSB of RF Frequency error */
    f_err_tc |= (read_reg(lora, REG_FEI_MSB) & 0xf) << 16;
    is_negative = (f_err_tc & 0x80000);
    if (is_negative)
    {
        f_err_tc &= ~(0x80000);
    }
    if (LORA_CHIP == SX1272)
    {
        //original formula: f_err = ((uint32_t)f_err_tc * (1U << 24)) / f_xtal;
        //re-write due to potential overflow of uint32_t
        f_err = (1U << 24) / (f_xtal / 1000);
        f_err = (f_err_tc * f_err) / 1000;
    }
    else if (LORA_CHIP == SX1276)
    {
        bw_hz = get_modem_config_signal_bandwidth_hz(lora);
        //original formula: f_err = (((uint32_t)f_err_tc * (1U << 24)) / f_xtal) * (bw_khz / 500);
        //re-write due to potential overflow of uint32_t
        f_err = (1U << 24) / (f_xtal / 1000);
        f_err = (f_err_tc * f_err) / 1000;
        f_err = (f_err * bw_hz) / 500;
    }
    if (f_err > 0x80000000)
    {
        //too big to convert to int32_t
        f_err = 0x80000000;
    }
    f_err_i = (int32_t)f_err;
    if (is_negative)
    {
        f_err_i = -f_err_i;
    }
    return f_err_i;
}
static inline uint8_t get_rssi_wideband(LORA* lora)
{
    /* Wideband RSSI measurement used to locally generate a
        random number */
    return read_reg(lora, REG_RSSI_WIDEBAND);
}
static inline void set_detection_optimize(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_DETECT_OPTIMIZE);
    /* LoRa detection Optimize
        0x03 - SF7 to SF12
        0x05 - SF6 */
    reg = setbits(reg, 2, 3, DETECTION_OPTIMIZE);
    write_reg(lora, REG_DETECT_OPTIMIZE, reg);
}
static inline uint8_t get_detection_optimize(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_DETECT_OPTIMIZE);
    /* LoRa detection Optimize
        0x03 - SF7 to SF12
        0x05 - SF6 */
    return (reg >> 0) & 7;
}
static inline void set_invert_i_and_q_signals(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_INVERT_I_Q);
    /* Invert the LoRa I and Q signals
        0 - normal mode
        1 - I and Q signals are inverted */
    reg = setbits(reg, 6, 1, LORA_INVERT_IQ);
    write_reg(lora, REG_INVERT_I_Q, reg);
}
static inline uint8_t get_invert_i_and_q_signals(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_INVERT_I_Q);
    /* Invert the LoRa I and Q signals
        0 - normal mode
        1 - I and Q signals are inverted */
    return (reg >> 6) & 1;
}
static inline void set_detection_threshold(LORA* lora)
{
    /* LoRa detection threshold
        0x0A - SF7 to SF12
        0x0C - SF6 */
    write_reg(lora, REG_DETECTION_THRESHOLD, DETECTION_THRESHOLD);
}
static inline uint8_t get_detection_threshold(LORA* lora)
{
    /* LoRa detection threshold
        0x0A - SF7 to SF12
        0x0C - SF6 */
    return read_reg(lora, REG_DETECTION_THRESHOLD);
}
static inline void set_sync_word(LORA* lora)
{
    /* LoRa Sync Word
        Value 0x34 is reserved for LoRaWAN networks */
    write_reg(lora, REG_SYNC_WORD, SX127X_SYNC_WORD);
}
static inline uint8_t get_sync_word(LORA* lora)
{
    /* LoRa Sync Word
        Value 0x34 is reserved for LoRaWAN networks */
    return read_reg(lora, REG_SYNC_WORD);
}
static inline uint32_t max(int32_t a, int32_t b)
{
    return a > b ? a : b;
}
static inline uint32_t ceil(uint32_t exp, uint32_t fra)
{
    return (fra > 0) ? exp+1 : exp;
}
static inline uint32_t get_symbol_length_ms(LORA* lora)
{
    uint32_t bw, sf, r_s, t_s;
    bw = get_modem_config_signal_bandwidth_hz(lora);
    sf = LORA_SPREADING_FACTOR;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * DOMAIN_US) / r_s;
    return t_s / 1000;
}
static inline uint32_t get_symbol_length_ms_ext(LORA* lora)
{
    uint32_t bw, sf, r_s, t_s;
    bw = get_modem_config_signal_bandwidth_hz_ext(lora, LORA_SIGNAL_BANDWIDTH);
    sf = LORA_SPREADING_FACTOR;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * DOMAIN_US) / r_s;
    return t_s / 1000;
}
static inline uint32_t get_pkt_time_on_air_ms_common(LORA* lora/*, bool tx*/, uint8_t pl)
{
    uint32_t bw, sf, r_s, t_s, /*pl,*/ ih, de, crc, cr, exp, fra, divisor; int32_t dividend;
    uint32_t n_preamble, n_payload, t_preamble, t_payload, t_packet_ms;

    bw = get_modem_config_signal_bandwidth_hz(lora);
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

    dividend = (8*pl) - (4*sf) + 28 + (16*crc) - (20*ih);
    divisor  = (4 * (sf - (2*de)));

    if (dividend < 0)
    {
        // number of payload symbols
        n_payload = 8 + 0;
    }
    else
    {
        exp = dividend / divisor;
        //get 1st digit of fractional part
        fra = ((dividend * 10) / divisor) - (exp * 10);
        // number of payload symbols
        n_payload = 8 + max(ceil(exp, fra) * (cr+4), 0);
    }
    // payload duration
    t_payload = n_payload * t_s;
    // total packet time on air
    t_packet_ms = (t_preamble + t_payload) / 1000;
    return t_packet_ms;
}
static inline uint32_t get_pkt_time_on_air_ms(LORA* lora)
{
    return get_pkt_time_on_air_ms_common(lora, get_payload_length(lora));
}
static inline uint32_t get_pkt_time_on_air_max_ms(LORA* lora)
{
    return get_pkt_time_on_air_ms_common(lora, LORA_MAX_PAYLOAD_LEN);
}
static inline bool set_pa_dac(LORA* lora, uint8_t pa_dac)
{
    uint8_t reg_pa_dac_addr;
    switch(LORA_CHIP)
    {
    case SX1272:
        reg_pa_dac_addr = SX1272_REG_PA_DAC;
        break;
    case SX1276:
        reg_pa_dac_addr = SX1276_REG_PA_DAC;
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        return false;
    }
    /* Enables the +20 dBm option on PA_BOOST pin
       0x04 - Default value
       0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
    if (pa_dac == 0x04)
    {
        pa_dac |= 0x80;
        write_reg(lora, reg_pa_dac_addr, pa_dac);
        return true;
    }
    else if (pa_dac == 0x07)
    {
        pa_dac |= 0x80;
        write_reg(lora, reg_pa_dac_addr, pa_dac);
        return true;
    }
    return false;
}
static inline uint8_t get_pa_dac(LORA* lora)
{
    uint8_t reg_pa_dac, reg_pa_dac_addr;
    switch(LORA_CHIP)
    {
    case SX1272:
        reg_pa_dac_addr = SX1272_REG_PA_DAC;
        break;
    case SX1276:
        reg_pa_dac_addr = SX1276_REG_PA_DAC;
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        return 0xFF;
    }
    /* Enables the +20 dBm option on PA_BOOST pin
       0x04 - Default value
       0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
    reg_pa_dac = read_reg(lora, reg_pa_dac_addr);
    return reg_pa_dac & 7;
}
static inline bool set_pa_dac_high_power_20_dBm(LORA* lora)
{
    /* Enables the +20 dBm option on PA_BOOST pin
       0x04 - Default value
       0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
    return set_pa_dac(lora, 0x07);
}

#if (LORA_DEBUG) && (LORA_DEBUG_DO_SPI_TEST)
static inline void spi_test(LORA* lora)
{
    const int test_sel = 0, iter = 255;
    uint8_t reg = 0, cnt = 0, chip, i;

    for (i = 0; i < iter; ++i)
    {
        if (test_sel == 0)
        {
            reg = read_reg(lora, REG_OP_MODE);
            printf("[sx127x] [info] REG_OP_MODE 0x%02x cnt:%d\n", reg, cnt);
            sleep_ms(100);
            write_reg(lora, REG_OP_MODE, cnt);
            sleep_ms(100);
            if (cnt++ >= 7) cnt = 0;
        }
        else if (test_sel == 1)
        {
            reg = read_reg(lora, REG_IRQ_FLAGS);
            sleep_ms(10);
            printf("[sx127x] [info] REG_IRQ_FLAGS 0x%02x\n", reg);
        }
        else if (test_sel == 2)
        {
            chip = get_version(lora);
            sleep_ms(100);
            printf("[sx127x] [info] chip %d\n", chip);
        }
    }
}
#endif

static inline void hw_reset(LORA* lora)
{
    SYSTIME uptime;
    LORA_CONFIG* config = &lora->config;
    SYS_VARS* sys_vars = lora->sys_vars;
    uint32_t ms_passed, rem_por_delay_ms;

    if (sys_vars->por_completed)
    {
        /* 7.2.2. Manual Reset */
        if (LORA_DO_MANUAL_RESET_AT_OPEN)
        {
            if      (LORA_CHIP == SX1272) gpio_set_pin  (config->reset_pin);
            else if (LORA_CHIP == SX1276) gpio_reset_pin(config->reset_pin);
            sleep_ms(2);
            if      (LORA_CHIP == SX1272) gpio_reset_pin(config->reset_pin);
            else if (LORA_CHIP == SX1276) gpio_set_pin  (config->reset_pin);
            sleep_ms(10);
#if (LORA_DEBUG)
            printf("[sx127x] [info] manual reset completed\n");
#endif
        }
    }
    else
    {
        //7.2.1. POR reset
        get_uptime(&uptime);
        ms_passed = (uptime.sec * 1000) + (uptime.usec / 1000);
        if (ms_passed < POR_RESET_MS)
        {
            rem_por_delay_ms = POR_RESET_MS-ms_passed;
            sleep_ms(rem_por_delay_ms);
#if (LORA_DEBUG)
            printf("[sx127x] [info] POR reset completed (remaining POR delay %d ms)\n", rem_por_delay_ms);
#endif
        }
        sys_vars->por_completed = true;
    }
}

static inline void clear_sys_vars(SYS_VARS* sys_vars)
{
    memset(sys_vars, 0, sizeof(SYS_VARS));
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

bool loras_hw_open(LORA* lora)
{
    mode_t mode;
    uint32_t rx_timeout_symb;
    uint8_t chip;

    if (!set_sys_vars(lora))
    {
        return false;
    }

    hw_reset(lora);

#if (LORA_DEBUG) && (LORA_DEBUG_DO_SPI_TEST)
    spi_test(lora);
#endif

    /* check for version */
    chip = get_version(lora);
    if (chip != LORA_CHIP)
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }

    /* set LoRa Mode (can be modified only in Sleep mode) */
    mode = get_mode(lora);
    if (mode != SLEEP)
    {
        set_mode(lora, SLEEP);
    }

    /* LongRangeMode:
        0 - FSK/OOK Mode
        1 - LoRaTM Mode */
    set_long_range_mode(lora, 1);

    /* This bit operates when device is in Lora mode; if set it allows
        access to FSK registers page located in address space
        (0x0D:0x3F) while in LoRa mode.
        0 - Access LoRa registers page 0x0D: 0x3F
        1 - Access FSK registers page (in mode LoRa) 0x0D: 0x3F */
    set_access_shared_reg(lora, 0);

    /* RF carrier frequency */
    set_rf_carrier_frequency(lora);

#if (LORA_CHIP == SX1276)
    /* Access Low Frequency Mode registers
        0 - High Frequency Mode (access to HF test registers)
        1 - Low  Frequency Mode (access to LF test registers) */
    /* The registers in the address space from 0x61 to 0x73 are specific
       for operation in the lower frequency bands (below 525 MHz), or in
       the upper frequency bands (above 779 MHz). */
    /* NOTE: For SX1272: 860 - 1020 MHz (Table 7)
       NOTE: For SX1276:
             Band 1 (HF) 862 - 1020 MHz (Table 32)
             Band 2 (LF) 410 - 525  MHz (Table 32)
             Band 3 (LF) 137 - 175  MHz (Table 32) */
    if (LORA_RF_CARRIER_FREQUENCY_MHZ <= 525)
    {
        set_low_frequency_mode_on(lora, 1);
    }
    else if (LORA_RF_CARRIER_FREQUENCY_MHZ >= 862)
    {
        set_low_frequency_mode_on(lora, 0);
    }
    else
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }

    /* [agc_reference_level] Sets the floor reference for all AGC thresholds:
            AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
            SNR = 8dB, fixed value
       [agc_step1] Defines the 1st AGC Threshold
       [agc_step2] Defines the 2nd AGC Threshold
       [agc_step3] Defines the 3rd AGC Threshold
       [agc_step4] Defines the 4th AGC Threshold
       [agc_step5] Defines the 5th AGC Threshold
       [pll_bandwidth] Controls the PLL bandwidth:
            00 - 75 kHz     10 - 225 kHz
            01 - 150 kHz    11 - 300 kHz */
    set_frequency_additional_registers(lora);
#endif

    if (SX127X_ENABLE_HIGH_OUTPUT_POWER)
    {
        if (!set_pa_dac_high_power_20_dBm(lora))
        {
            return false;
        }
    }

#if (LORA_CHIP == SX1272)
    set_pa_sx1272(lora);
#elif (LORA_CHIP == SX1276)
    set_pa_sx1276(lora);
#endif

    set_ocp(lora);

    set_lna(lora);

    /* [fifo_addr_ptr] SPI interface address pointer in FIFO data buffer. */
    /* [fifo_tx_base_addr] write base address in FIFO data buffer for TX modulator */
    /* [fifo_rx_base_addr] read base address in FIFO data buffer for RX demodulator */
    set_fifo_address_pointers(lora);

    // unmask all irqs
    set_reg_irq_flags_mask_all(lora);

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    if (LORA_CHIP == SX1272) //todo: all if to #if??
    {
        /* Set modem parameters */
        set_modem_config(lora, LOW_DATA_RATE_OPTIMIZE);
    }
    else if (LORA_CHIP == SX1276)
    {
        /* [low_data_rate_optimize] LowDataRateOptimize:
               0 - Disabled
               1 - Enabled; mandated for when the symbol length exceeds 16 ms */
        set_modem_config(lora, get_symbol_length_ms_ext(lora) > 16);
    }

    if (!LORA_CONTINUOUS_RECEPTION_ON)
    {
        /* In this mode, the modem searches for a preamble during a given period
           of time. If a preamble hasn’t been found at the end of the time
           window, the chip generates the RxTimeout interrupt and goes back to
           Standby mode. The length of the reception window (in symbols) is
           defined by the RegSymbTimeout register and should be in the range of
           4 (minimum time for the modem to acquire lock on a preamble) up to
           1023 symbols. */
        /* [symb_timeout] RX operation time-out value expressed as number of symbols */
        rx_timeout_symb = rx_timeout_ms_to_symb(lora, lora->rx_timeout_ms);
        if (rx_timeout_symb < TIMEOUT_MIN_SYMB)
        {
            rx_timeout_symb = TIMEOUT_MIN_SYMB;
        }
        else if (rx_timeout_symb > TIMEOUT_MAX_SYMB)
        {
            rx_timeout_symb = TIMEOUT_MAX_SYMB;
        }
        set_rx_timeout(lora, rx_timeout_symb);
    }

    /* The transmitted preamble length may be changed by setting
       the register RegPreambleMsb and RegPreambleLsb from 6 to 65535, yielding
       total preamble lengths of 6 + 4 to 65535 + 4 symbols.
       Preamble length MSB+LSB, = PreambleLength + 4.25 Symbols
       See Section 4.1.1.6 for more details. */
    set_preamble_length(lora);

    /* Maximum payload length; if header payload length exceeds value a header
       CRC error is generated. Allows filtering of packet with a bad size. */
    set_max_payload_length(lora);

    set_symbol_periods_between_frequency_hops(lora);


    if (LORA_CHIP == SX1276)
    {
        set_ppm_correction(lora);
    }

    /* [detection_optimize] LoRa detection Optimize
            0x03 - SF7 to SF12
            0x05 - SF6 */
    set_detection_optimize(lora);

    /* Invert the LoRa I and Q signals
        0 - normal mode
        1 - I and Q signals are inverted */
    set_invert_i_and_q_signals(lora);

    /* [detection_threshold] LoRa detection threshold
            0x0A - SF7 to SF12
            0x0C - SF6 */
    set_detection_threshold(lora);

    set_sync_word(lora);
    return true;
}

void loras_hw_close(LORA* lora)
{
    set_mode(lora, SLEEP);
}

void loras_hw_sleep(LORA* lora)
{
    mode_t mode = get_mode(lora);
    if (mode != SLEEP)
    {
        set_mode(lora, SLEEP);
    }
}

void loras_hw_tx_async_wait(LORA* lora)
{
#if (LORA_DEBUG)
    mode_t mode;
#endif
    uint8_t irq_flags;

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    //NOTE: Do not know if packet is already sent (state is TX or STDBY)
    //      => do not check mode
#endif

    // wait for tx_done IRQ
    irq_flags = get_reg_irq_flags_all(lora);
    if (get_reg_irq_flags(lora, IRQ_TX_DONE, irq_flags))
    {
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else
    {
        //no irq
        error(ERROR_OK);
        return;
    }

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    // Auto-go back to Standby mode
    /* When activated the SX1272/73 powers all remaining blocks required
       for transmit, ramps the PA, transmits the packet and returns to Standby mode */
#if (LORA_DEBUG)
    mode = get_mode(lora);
    if (mode != STDBY)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
#endif
    // Go back from STDBY to SLEEP
    set_mode(lora, SLEEP);
    error(ERROR_OK);
}

void loras_hw_tx_async(LORA* lora, IO* io)
{
#if (LORA_DEBUG)
    mode_t mode;
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
        printf("[sx127x] [info] new lora_tx while RX is ON (continuous reception) => stop RX\n");
#endif
        // Go back to SLEEP
        set_mode(lora, SLEEP);
        lora->rxcont_mode_started = false;
    }

#if (LORA_DEBUG)
    mode = get_mode(lora);
    if (mode != SLEEP)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
#endif

    /* See Figure 18. Startup Process */
    set_mode(lora, STDBY);

#if (LORA_CHIP == SX1276)
    gpio_set_pin(lora->config.rxtx_ext_pin);
#endif

    // tx init
    /* LoRaTM Transmit Data FIFO Filling
        In order to write packet data into FIFO user should:
        1 Set FifoAddrPtr to FifoTxBaseAddrs.
        2 Write PayloadLength bytes to the FIFO (RegFifo) */
    set_fifo_address_pointers(lora);

    // 2 Write PayloadLength bytes to the FIFO (RegFifo)
    if (!write_fifo(lora, io))
    {
        return;
    }

    // The register RegPayloadLength indicates the size of the memory location to be transmitted
    set_payload_length(lora, io->data_size);

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    // mode request - TX
    set_mode(lora, TX);

    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    error(ERROR_OK);

    timer_start_ms(lora->timer_txrx_timeout, lora->tx_timeout_ms);
}

void loras_hw_rx_async_wait(LORA* lora)
{
#if (LORA_DEBUG)
    mode_t mode;
#endif
    uint8_t irq_flags, payload_size, fifo_rx_current_addr;

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    mode = get_mode(lora);
    if (LORA_CONTINUOUS_RECEPTION_ON)
    {
        if (mode != RXCONTINUOUS)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
    }
    //else => Single mode => When activated the SX1272/73 powers all remaining blocks
    //required for reception, remains in this state until a valid packet has been received
    //and then returns to Standby mode.
    //NOTE: Do not know if packet is received (state RXSINGLE or STDBY)
    //      => do not check state
#endif

    /* 2 Upon reception of a valid header CRC the RxDone interrupt is set. The radio remains
        in RXCONT mode waiting for the next RX LoRaTM packet. */
    irq_flags = get_reg_irq_flags_all(lora);
    // wait for RX_DONE IRQ
    if (get_reg_irq_flags(lora, IRQ_RX_DONE, irq_flags))
    {
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else if (!LORA_CONTINUOUS_RECEPTION_ON && get_reg_irq_flags(lora, IRQ_RX_TIMEOUT, irq_flags))
    {
        error(ERROR_TIMEOUT);
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
        set_mode(lora, SLEEP);
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

    /* 3 The PayloadCrcError flag should be checked for packet integrity. */
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    if (LORA_IMPLICIT_HEADER_MODE_ON || crc_is_present_in_rcvd_pkt_header(lora))
    {
        if (get_reg_irq_flags(lora, IRQ_PAYLOAD_CRC_ERROR, irq_flags))
        {
            error(LORA_ERROR_RX_INCORRECT_PAYLOAD_CRC);
            lora->status = LORA_STATUS_TRANSFER_COMPLETED;
            if (!LORA_CONTINUOUS_RECEPTION_ON)
                set_mode(lora, SLEEP);
#if (LORA_DO_ERROR_COUNTING)
            ++lora->stats_rx.crc_err_num;
#endif
            return;
        }
    }

    /* 4 If packet has been correctly received the FIFO data buffer can be read (see below). */
    /*      In order to retrieve received data from FIFO the user must ensure that ValidHeader, PayloadCrcError,
            RxDone and RxTimeout interrupts in the status register RegIrqFlags are not asserted */
    /* NOTE:    In continuous RX mode, opposite to the single RX mode, the RxTimeout interrupt will never
                occur and the device will never go in Standby mode automatically. */
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    if (!LORA_IMPLICIT_HEADER_MODE_ON && !get_reg_irq_flags(lora, IRQ_VALID_HEADER, irq_flags))
    {
        error(LORA_ERROR_RX_INVALID_HEADER);
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
        if (!LORA_CONTINUOUS_RECEPTION_ON)
            set_mode(lora, SLEEP);
#if (LORA_DO_ERROR_COUNTING)
        //tbd
#endif
        return;
    }

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    payload_size = get_latest_pkt_rcvd_payload_size(lora);

    // read FIFO
    if (payload_size > 0)
    {
        get_fifo_rx_current_addr(lora, &fifo_rx_current_addr);
        set_fifo_addr_ptr       (lora,  fifo_rx_current_addr);
        lora->io->data_size = payload_size;
        if (!read_fifo(lora, lora->io))
        {
            return;
        }
    }
    else
    {
        error(LORA_ERROR_RX_INCORRECT_PAYLOAD_LEN);
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
        if (!LORA_CONTINUOUS_RECEPTION_ON)
            set_mode(lora, SLEEP);
#if (LORA_DO_ERROR_COUNTING)
        //tbd
#endif
        lora->io->data_size = 0;
        return;
    }

    if (!LORA_CONTINUOUS_RECEPTION_ON)
    {
        // Go back to SLEEP
        set_mode(lora, SLEEP);
    }
    /* else => LORA_CONTINUOUS_RECEPTION
       NOTE: In continuous receive mode, the modem scans the channel continuously for a preamble. */
    error(ERROR_OK);
}

void loras_hw_rx_async(LORA* lora, IO* io)
{
#if (LORA_DEBUG)
    uint8_t mode;
#endif
    uint8_t mode_sel, start_rx;

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_COMPLETED)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    mode = get_mode(lora);
    if (!LORA_CONTINUOUS_RECEPTION_ON || !lora->rxcont_mode_started)
    {
        if (mode != SLEEP)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
    }
    else
    {
        if (mode != RXCONTINUOUS)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
    }
#endif

    /* LoRaTM Receive Data FIFO Filling
        In order to write packet data into FIFO user should:
        1 Set FifoAddrPtr to FifoTxBaseAddrs.
        2 Write PayloadLength bytes to the FIFO (RegFifo) */
    set_fifo_address_pointers(lora);

    /* Payload length in bytes. The register needs to be set in
       implicit header mode for the expected packet length.
       A 0 value is not permitted */
    if (LORA_IMPLICIT_HEADER_MODE_ON)
    {
        set_payload_length(lora, io->data_size);
    }

    start_rx = 0;

    if (!LORA_CONTINUOUS_RECEPTION_ON)
    {
        start_rx = 1;
        mode_sel = RXSINGLE;
    }
    else if (!lora->rxcont_mode_started)
    {
        start_rx = 1;
        mode_sel = RXCONTINUOUS;
        lora->rxcont_mode_started = true;
    }

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    if (start_rx)
    {
#if (LORA_DEBUG)
        /* 1 Whilst in Sleep or Standby mode select RXCONT mode. */
        mode = get_mode(lora);
        if (mode != SLEEP)
        {
            error(ERROR_INVALID_STATE);
            return;
        }
#endif
        /* See Figure 18. Startup Process */
        set_mode(lora, STDBY);

#if (LORA_CHIP == SX1276)
        gpio_reset_pin(lora->config.rxtx_ext_pin);
#endif

        //NOTE: no delay between changing states
        set_mode(lora, mode_sel);
    }

    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    error(ERROR_OK);
    //NOTE: software timer_txrx_timeout is used only for LORA_CONTINUOUS_RECEPTION
    //      (for LORA_SINGLE_RECEPTION default RX_TIMEOUT irq is used).
    if (LORA_CONTINUOUS_RECEPTION_ON)
    {
        timer_start_ms(lora->timer_txrx_timeout, lora->rx_timeout_ms);
    }
}

uint32_t loras_hw_get_stat(LORA* lora, LORA_STAT stat)
{
    switch (stat)
    {
    case PKT_TIME_ON_AIR_MS:
        return /*uint32_t*/get_pkt_time_on_air_ms(lora);
    case PKT_TIME_ON_AIR_MAX_MS:
        return /*uint32_t*/get_pkt_time_on_air_max_ms(lora);
    case TX_OUTPUT_POWER_DBM:
        return /*  int8_t*/get_tx_output_power_dbm(lora);
    case RX_RSSI_CUR_DBM:
        return /* int16_t*/get_rssi_cur_dbm(lora);
    case RX_RSSI_LAST_PKT_RCVD_DBM:
        return /* int16_t*/get_rssi_last_pkt_rcvd_dbm(lora);
    case RX_SNR_LAST_PKT_RCVD_DB:
        return /*  int8_t*/get_snr_last_pkt_rcvd_db(lora);
    case RX_RF_FREQ_ERROR_HZ:
        return /* int32_t*/get_rf_freq_error_hz(lora);
    default:
        error (ERROR_NOT_SUPPORTED);
        return 0;
    }
}

