/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#ifndef SX127X_H
#define SX127X_H

#include "spi.h"
#include "stdio.h"
#include <string.h>

// Several forward declarations needed
static inline uint8_t get_mode(LORA* lora);

static inline void write_reg_common(LORA* lora, uint8_t address, uint8_t* data, uint16_t size)
{
    LORA_SPI* spi = &lora->spi;
    ((uint8_t*)io_data(spi->io))[0] = address | 0x80;
    memcpy(io_data(spi->io)+1, data, size);
    spi->io->data_size = size+1;

    gpio_reset_pin(lora->usr_config.spi_cs_pin);
#if defined(STM32)
    //todo: could not use spi_send/spi_write_sync => use spi_read_sync instead
    spi_read_sync(spi->port, spi->io, spi->io->data_size);
#elif defined (MK22)
    spi_write_sync(spi->port, spi->io);
#else
#error "unknown target" 
#endif  
    gpio_set_pin(lora->usr_config.spi_cs_pin);
}
static inline void read_reg_common(LORA* lora, uint8_t address, uint8_t* data, uint16_t size)
{   
    LORA_SPI* spi = &lora->spi;
    ((uint8_t*)io_data(spi->io))[0] = address & 0x7F;
    spi->io->data_size = size+1;

    gpio_reset_pin(lora->usr_config.spi_cs_pin);
    spi_read_sync(spi->port, spi->io, spi->io->data_size);
    gpio_set_pin(lora->usr_config.spi_cs_pin);

    memcpy(data, io_data(spi->io)+1, size);
}
static inline void write_reg(LORA* lora, uint8_t address, uint8_t data)
{   
    write_reg_common(lora, address, &data, 1);
}
static inline uint8_t read_reg(LORA* lora, uint8_t address)
{   
    uint8_t reg;
    read_reg_common(lora, address, &reg, 1);
    return reg;
}

static inline uint32_t get_modem_config_signal_bandwidth_hz(LORA* lora);

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

/* Function setbits(x,p,n,y) that returns x with the n bits that 
  begin at position p set to the rightmost n bits of y,leaving the other bits unchanged.*/
static inline unsigned setbits(unsigned x,int p,int n,unsigned y)
{
    int shift = p - n + 1;
    unsigned mask = (1 << n) - 1;
    return ((((x >> shift) ^ y) & mask) << shift) ^ x;
}

/* Semtech ID relating the silicon revision */
#define REG_VERSION         (0x42)
#define VER_SX1272          (0x22)
#define VER_SX1276          (0x12)

static inline uint8_t get_version(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_VERSION); 
    switch (reg)
    {
        case VER_SX1272: return LORA_CHIP_SX1272;
        case VER_SX1276: return LORA_CHIP_SX1276;
        default:break;
    }
    return LORA_CHIP_UNKNOWN;
}

#define REG_FIFO                        (0x00)
static inline int write_fifo(LORA* lora, uint8_t* data, uint8_t size)
{
    /* FIFO is cleared an not accessible when device is in SLEEP mode */
    if (get_mode(lora) == SLEEP)
        return -1;
    write_reg_common(lora, REG_FIFO, data, size);
    return 0;
}
static inline int read_fifo(LORA* lora, uint8_t* data, uint8_t size)
{
    /* FIFO is cleared an not accessible when device is in SLEEP mode */
    if (get_mode(lora) == SLEEP)
        return -1;
    read_reg_common(lora, REG_FIFO, data, size);
    return 0;
}

/* Common Register Settings */
#define REG_OP_MODE                     (0x01)
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
        1 - Low Frequency Mode (access to LF test registers) */
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

#define REG_FR_MSB                      (0x06)
#define REG_FR_MIB                      (0x07)
#define REG_FR_LSB                      (0x08)
static inline void set_rf_carrier_frequency(LORA* lora, uint32_t frequency_mhz)
{
    uint32_t value, freq_osc, mode;
    /* Resolution is 61.035 Hz if F(XOSC) = 32 MHz. Default value is
        0xe4c000 = 915 MHz. Register values must be modified only
        when device is in SLEEP or STANDBY mode. */
    mode = get_mode(lora);
    if (mode == SLEEP || mode == STDBY) 
    {
        //Crystal oscillator frequency (see Table 7 Frequency Synthesizer Specification)
        freq_osc = 32;//MHz
        value = (frequency_mhz * (1<<19)) / freq_osc;
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

/* Low/High Frequency Additional Registers - SX1276 specific */
#define REG_AGC_REF_LF                  (0x61)
#define REG_AGC_THRESH1_LF              (0x62)
#define REG_AGC_THRESH2_LF              (0x63)
#define REG_AGC_THRESH3_LF              (0x64)
#define REG_PLL_LF                      (0x70)

static inline void set_frequency_additional_registers(LORA* lora,
    uint8_t agc_reference_level,
    uint8_t agc_step1,
    uint8_t agc_step2,
    uint8_t agc_step3,
    uint8_t agc_step4,
    uint8_t agc_step5,
    uint8_t pll_bandwidth)
{
    uint8_t reg;
    reg = read_reg(lora, REG_AGC_REF_LF);
    /* Sets the floor reference for all AGC thresholds:
       AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
       SNR = 8dB, fixed value */
    reg = setbits(reg, 5, 6, agc_reference_level);
    write_reg(lora, REG_AGC_REF_LF, reg);   

    reg = read_reg(lora, REG_AGC_THRESH1_LF);
    /* Defines the 1st AGC Threshold */
    reg = setbits(reg, 4, 5, agc_step1);
    write_reg(lora, REG_AGC_THRESH1_LF, reg); 

    reg = read_reg(lora, REG_AGC_THRESH2_LF);
    /* Defines the 2nd AGC Threshold */
    reg = setbits(reg, 7, 4, agc_step2);
    /* Defines the 3rd AGC Threshold */
    reg = setbits(reg, 3, 4, agc_step3);
    write_reg(lora, REG_AGC_THRESH2_LF, reg);

    reg = read_reg(lora, REG_AGC_THRESH3_LF);
    /* Defines the 4th AGC Threshold */
    reg = setbits(reg, 7, 4, agc_step4);
    /* Defines the 5th AGC Threshold */
    reg = setbits(reg, 3, 4, agc_step5);
    write_reg(lora, REG_AGC_THRESH3_LF, reg);

    reg = read_reg(lora, REG_PLL_LF);
    /* Controls the PLL bandwidth:
        00 - 75 kHz     10 - 225 kHz
        01 - 150 kHz    11 - 300 kHz */
    reg = setbits(reg, 7, 2, pll_bandwidth);
    write_reg(lora, REG_PLL_LF, reg);
}
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


/* Register for RF */
#define REG_PA_CONFIG                   (0x09)
#define REG_PA_RAMP                     (0x0A)
static inline void set_pa_sx1272(LORA* lora, uint8_t pa_select,
    uint8_t tx_output_power_dbm)
{
    uint8_t reg;
    reg = read_reg(lora, REG_PA_CONFIG);
    /* Selects PA output pin:
       0 RFIO pin. Output power is limited to 13 dBm.
       1 PA_BOOST pin. Output power is limited to 20 dBm */
    reg = setbits(reg, 7, 1, pa_select);
    /* power amplifier max output power:
        Pout = 2 + OutputPower(3:0) on PA_BOOST.
        Pout = -1 + OutputPower(3:0) on RFIO. */
    reg = setbits(reg, 3, 4, tx_output_power_dbm);
    write_reg(lora, REG_PA_CONFIG, reg); 

    reg = read_reg(lora, REG_PA_RAMP);
    /* LowPnTxPllOff:
        1 Low consumption PLL is used in receive and transmit mode
        0 Low consumption PLL in receive mode, low phase noise
        PLL in transmit mode. */
    reg = setbits(reg, 4, 1, LORA_LOW_PN_TX_PLL_OFF);
    /* Rise/Fall time of ramp up/down in FSK
        0000 - 3.4 ms       0001 - 2 ms
        0010 - 1 ms         0011 - 500 us
        0100 - 250 us       0101 - 125 us
        0110 - 100 us       0111 - 62 us
        1000 - 50 us        1001 - 40 us
        1010 - 31 us        1011 - 25 us
        1100 - 20 us        1101 - 15 us
        1110 - 12 us        1111 - 10 us */
    reg = setbits(reg, 3, 4, LORA_PA_RAMP);
    write_reg(lora, REG_PA_RAMP, reg);    
}
static inline void set_pa_sx1276(LORA* lora, 
    uint8_t pa_select, uint8_t max_power, 
    uint8_t tx_output_power_dbm)
{
    uint8_t reg;
    reg = read_reg(lora, REG_PA_CONFIG);
    /* Selects PA output pin
       0 - RFO pin. Output power is limited to +14 dBm.
       1 - PA_BOOST pin. Output power is limited to +20 dBm */
    reg = setbits(reg, 7, 1, pa_select);
    /* Select max output power: Pmax=10.8+0.6*MaxPower [dBm] */
    reg = setbits(reg, 6, 3, max_power);
    /* Pout=Pmax-(15-OutputPower) if PaSelect = 0 (RFO pin)
       Pout=17-(15-OutputPower) if PaSelect = 1 (PA_BOOST pin) */
    reg = setbits(reg, 3, 4, tx_output_power_dbm);
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
    reg = setbits(reg, 3, 4, LORA_PA_RAMP);
    write_reg(lora, REG_PA_RAMP, reg);    
}
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
static inline uint8_t sx1276_get_max_power(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_PA_CONFIG);
    /* Select max output power: Pmax=10.8+0.6*MaxPower [dBm] */
    return (reg >> 4) & 7;   
}
static inline uint8_t get_pa_dac(LORA* lora);
static inline int8_t get_tx_output_power_dbm(LORA* lora)
{
    int8_t tx_output_power_dbm, pout_dbm;
    uint8_t pa_dac;

    tx_output_power_dbm = get_tx_output_power(lora);
    pa_dac = get_pa_dac(lora);
    if (lora->usr_config.chip == LORA_CHIP_SX1272)
    {
        /* [pa_select] Selects PA output pin:
                0 RFIO pin. Output power is limited to 13 dBm.
                1 PA_BOOST pin. Output power is limited to 20 dBm */
        if (lora->sys_config.pa_select == 1)
        {
            /* The SX1272/73 has a high power +20 dBm capability on PA_BOOST pin
               (see 5.4.3. High Power +20 dBm Operation) */
            /* Enables the +20 dBm option on PA_BOOST pin
               0x04 - Default value
               0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
            if      (pa_dac == 7)   pout_dbm = 20;
            /* power amplifier max output power:
                Pout =  2 + OutputPower(3:0) on PA_BOOST.
                Pout = -1 + OutputPower(3:0) on RFIO. */
            else if (pa_dac == 4)   pout_dbm = 2 + tx_output_power_dbm;                
            else                    pout_dbm = -128;//error
        }
        else
        {
            pout_dbm = -1 + tx_output_power_dbm;
        }
    }
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
    {
        /* [pa_select] Selects PA output pin:
                0 RFIO pin. Output power is limited to 13 dBm.
                1 PA_BOOST pin. Output power is limited to 20 dBm */
        if (lora->sys_config.pa_select == 1)
        {
            /* The SX1276/77/78/79 have a high power +20 dBm capability on PA_BOOST pin
               see (5.4.3. High Power +20 dBm Operation) */
            /* Enables the +20 dBm option on PA_BOOST pin
               0x04 - Default value
               0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
            if      (pa_dac == 7)    pout_dbm = 20;
            /* Pout=17-(15-OutputPower) [dBm] */
            else if (pa_dac == 4)    pout_dbm = 17 - (15 - tx_output_power_dbm);
            else                     pout_dbm = -128;//error
        }
        else
        {
            /*  Pout=Pmax-(15-OutputPower)
                Pmax=10.8+0.6*MaxPower [dBm] */
            pout_dbm = (108 + 6 * sx1276_get_max_power(lora)) / 10;
        } 
    }
    else
    {
        pout_dbm = -128;
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

#define REG_OCP                         (0x0B)
static inline void set_ocp(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_OCP);
    /* Enables overload current protection (OCP) for PA:
        0 - OCP disabled
        1 - OCP enabled */
    reg = setbits(reg, 5, 1, LORA_OCP_ON);
    /*  Trimming of OCP current:
        Imax = 45+5*OcpTrim [mA] if OcpTrim <= 15 (120 mA) /
        Imax = -30+10*OcpTrim [mA] if 15 < OcpTrim <= 27 (130 to 240 mA)
        Imax = 240mA for higher settings
        Default Imax = 100mA */
    reg = setbits(reg, 4, 5, LORA_OCP_TRIM);
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
    /*  Trimming of OCP current:
        Imax = 45+5*OcpTrim [mA] if OcpTrim <= 15 (120 mA) /
        Imax = -30+10*OcpTrim [mA] if 15 < OcpTrim <= 27 (130 to 240 mA)
        Imax = 240mA for higher settings
        Default Imax = 100mA */
    return (reg >> 0) & 0x1f;  
}

#define REG_LNA                         (0x0C)
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
    reg = setbits(reg, 7, 3, LORA_LNA_GAIN);
    /*  LnaBoost:
        00 - Default LNA current
        11 - Boost on, 150% LNA current. */
    reg = setbits(reg, 1, 2, LORA_LNA_BOOST);
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

/* Lora page registers */
#define REG_FIFO_ADDR_PTR               (0x0D)
#define REG_FIFO_TX_BASE_ADDR           (0x0E)
#define REG_FIFO_RX_BASE_ADDR           (0x0F)
#define REG_FIFO_RX_CURRENT_ADDR        (0x10)

static inline void set_fifo_address_pointers(LORA* lora, 
                               uint8_t fifo_addr_ptr, uint8_t fifo_tx_base_addr, 
                               uint8_t fifo_rx_base_addr)
{
    /* SPI interface address pointer in FIFO data buffer. */
    write_reg(lora, REG_FIFO_ADDR_PTR, fifo_addr_ptr);
    /* write base address in FIFO data buffer for TX modulator */  
    write_reg(lora, REG_FIFO_TX_BASE_ADDR, fifo_tx_base_addr);
    /* read base address in FIFO data buffer for RX demodulator */  
    write_reg(lora, REG_FIFO_RX_BASE_ADDR, fifo_rx_base_addr);  
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

typedef enum _LORA_IRQ
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

#define REG_IRQ_FLAGS_MASK              (0x11)
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

#define REG_IRQ_FLAGS                   (0x12)
static inline void set_reg_irq_flags_clear(LORA* lora, irq_t irq)
{
    /* XXX interrupt: writing a 1 clears the IRQ */
    uint8_t reg;
    reg = read_reg(lora, REG_IRQ_FLAGS_MASK);
    reg = setbits(reg, (int)irq, 1, 1);
    write_reg(lora, REG_IRQ_FLAGS, reg);  
    //reg = read_reg(lora, REG_IRQ_FLAGS);//test
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


#define REG_RX_NB_BYTES                 (0x13)
static inline uint8_t get_latest_packet_received_payload_size(LORA* lora)
{
    return read_reg(lora, REG_RX_NB_BYTES);
}

#define REG_RX_HEADER_CNT_VALUE_MSB     (0x14)
#define REG_RX_HEADER_CNT_VALUE_LSB     (0x15)
static inline uint16_t get_valid_headers_received_num(LORA* lora)
{
    uint16_t value = 0;
    value |= read_reg(lora, REG_RX_HEADER_CNT_VALUE_LSB);
    value |= read_reg(lora, REG_RX_HEADER_CNT_VALUE_MSB) << 8;
    return value;  
}

#define REG_RX_PACKET_CNT_VALUE_MSB     (0x16)
#define REG_RX_PACKET_CNT_VALUE_LSB     (0x17)
static inline uint16_t get_valid_packets_received_num(LORA* lora)
{
    uint16_t value = 0;
    value |= read_reg(lora, REG_RX_PACKET_CNT_VALUE_LSB);
    value |= read_reg(lora, REG_RX_PACKET_CNT_VALUE_MSB) << 8;
    return value;     
}

#define REG_MODEM_STAT                  (0x18)
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

#define REG_PKT_SNR_VALUE               (0x19)
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

#define REG_PKT_RSSI_VALUE              (0x1A)
#define REG_RSSI_VALUE                  (0x1B)
static inline int16_t get_rssi_last_pkt_rcvd_dbm(LORA* lora)
{
    int16_t rssi_dbm; int8_t snr_db, snr_tc; uint8_t rssi_pkt;
    rssi_dbm = 0;  snr_db = 0;  snr_tc = 0;  rssi_pkt = 0;
    rssi_pkt = read_reg(lora, REG_PKT_RSSI_VALUE);
#if (LORA_DEBUG)
    printf("[sx127x] [info] rssi_pkt %u\n", rssi_pkt);
#endif
    snr_db = get_snr_last_pkt_rcvd_db(lora);
    if (lora->usr_config.chip == LORA_CHIP_SX1272)
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
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
    {
        /* Current RSSI value (dBm)
            RSSI[dBm] = - 139 + Rssi */
        rssi_dbm = -139 + (int16_t)rssi;
    }
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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

#define REG_HOP_CHANNEL                 (0x1C)
static inline uint8_t get_pll_failed_to_lock_while_tx_rx_cad_operation(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_HOP_CHANNEL);
    /* PLL failed to lock while attempting a TX/RX/CAD operation
        1 - PLL did not lock
        0 - PLL did lock */
    return (reg >> 7) & 1;     
}
static inline uint8_t get_crc_info_from_rcvd_pkt_header(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_HOP_CHANNEL);
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    return (reg >> 6) & 1;  
}
static inline uint8_t get_frequency_hopping_channel_current_value(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_HOP_CHANNEL);
    /* Current value of frequency hopping channel in use. */
    return (reg >> 0) & 0x3f;      
}

#define REG_MODEM_CONFIG1               (0x1D)
#define REG_MODEM_CONFIG2               (0x1E)
#define REG_MODEM_CONFIG3               (0x26)
static inline void set_modem_config(LORA* lora,
        uint8_t signal_bandwidth,
        uint8_t implicit_header_mode_on,
        uint8_t rx_payload_crc_on,
        uint8_t low_data_rate_optimize,
        uint8_t spreading_factor
     )
{
    uint8_t reg;
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
    {
        reg = read_reg(lora, REG_MODEM_CONFIG1);
        /* Signal bandwidth:
            00 - 125 kHz
            01 - 250 kHz
            10 - 500 kHz
            11 - reserved */
        reg = setbits(reg, 7, 2, signal_bandwidth);
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
        reg = setbits(reg, 2, 1, implicit_header_mode_on);
        /* Enable CRC generation and check on payload:
            0 - CRC disable
            1 - CRC enable
            If CRC is needed, RxPayloadCrcOn should be set:
            - in Implicit header mode: on Tx and Rx side
            - in Explicit header mode: on the Tx side alone (recovered from
            the header in Rx side) */
        reg = setbits(reg, 1, 1, rx_payload_crc_on);
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
        reg = setbits(reg, 7, 4, spreading_factor);
        /* TxContinuousMode:
            0 - normal mode, a single packet is sent
            1 - continuous mode, send multiple packets across the FIFO
                (used for spectral analysis) */
        reg = setbits(reg, 3, 1, LORA_TX_CONTINUOUS_MODE);
        /* AgcAutoOn:
            0 - LNA gain set by register LnaGain
            1 - LNA gain set by the internal AGC loop */
        reg = setbits(reg, 2, 1, LORA_AGC_AUTO_ON);
        write_reg(lora, REG_MODEM_CONFIG2, reg);
    }
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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
        reg = setbits(reg, 7, 4, signal_bandwidth);
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
        reg = setbits(reg, 0, 1, implicit_header_mode_on);
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
        reg = setbits(reg, 7, 4, spreading_factor);
        /* TxContinuousMode:
            0 - normal mode, a single packet is sent
            1 - continuous mode, send multiple packets across the FIFO
                (used for spectral analysis) */
        reg = setbits(reg, 3, 1, LORA_TX_CONTINUOUS_MODE);
        /* Enable CRC generation and check on payload:
            0 - CRC disable
            1 - CRC enable
            If CRC is needed, RxPayloadCrcOn should be set:
            - in Implicit header mode: on Tx and Rx side
            - in Explicit header mode: on the Tx side alone (recovered from
            the header in Rx side) */
        reg = setbits(reg, 2, 1, rx_payload_crc_on);
        write_reg(lora, REG_MODEM_CONFIG2, reg);

        reg = read_reg(lora, REG_MODEM_CONFIG3); 
        /* LowDataRateOptimize:
            0 - Disabled
            1 - Enabled; mandated for when the symbol length exceeds 16ms */
        reg = setbits(reg, 3, 1, low_data_rate_optimize);
        /* AgcAutoOn:
            0 - LNA gain set by register LnaGain
            1 - LNA gain set by the internal AGC loop */
        reg = setbits(reg, 2, 1, LORA_AGC_AUTO_ON);
        write_reg(lora, REG_MODEM_CONFIG3, reg);
    }
}
static inline uint8_t get_modem_config_signal_bandwidth(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG1);
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
    {
        /* Signal bandwidth:
            00 - 125 kHz
            01 - 250 kHz
            10 - 500 kHz
            11 - reserved */
        return (reg >> 6) & 3;
    }
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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

static inline uint32_t get_modem_config_signal_bandwidth_hz(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG1), bw;
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
    {
        /* Signal bandwidth:
            00 - 125 kHz            01 - 250 kHz
            10 - 500 kHz            11 - reserved */
        bw = (reg >> 6) & 3;
        switch (bw)
        {
            case 0:  return 125000; 
            case 1:  return 250000;
            case 2:  return 500000; 
            case 3:  return 0;  
            default: return 0;
        }
    }
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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
        bw = (reg >> 4) & 0xf;
        switch (bw)
        {
            case 0:  return 7800;    case 1: return 10400;
            case 2:  return 15600;   case 3: return 20800;
            case 4:  return 31250;   case 5: return 41700;
            case 6:  return 62500;   case 7: return 125000;
            case 8:  return 250000;  case 9: return 500000;
            default: return 0;
        }
    }
    return 0;
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
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
        return (reg >> 3) & 7;   
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
        return (reg >> 1) & 7;         
    return 0;
}
static inline uint8_t get_modem_config_implicit_header_mode_on(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_MODEM_CONFIG1);
    /* ImplicitHeaderModeOn:
        0 - Explicit Header mode
        1 - Implicit Header mode */
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
        return (reg >> 2) & 1;  
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
    {
        reg = read_reg(lora, REG_MODEM_CONFIG1);
        return (reg >> 1) & 1;
    }  
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
    {
        reg = read_reg(lora, REG_MODEM_CONFIG1);
        return (reg >> 0) & 1;   
    }
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
        reg = read_reg(lora, REG_MODEM_CONFIG2);
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
        reg = read_reg(lora, REG_MODEM_CONFIG3);
    else
        reg = 0;
    return (reg >> 2) & 1;
}

#define REG_PPM_CORRECTION            (0x27)
static inline void set_ppm_correction(LORA* lora)
{
    /* Data rate offset value, used in conjunction with AFC */
    write_reg(lora, REG_PPM_CORRECTION, LORA_PPM_CORRECTION);    
}
static inline uint8_t get_ppm_correction(LORA* lora)
{
    /* Data rate offset value, used in conjunction with AFC */
    return read_reg(lora, REG_PPM_CORRECTION);     
}

#define REG_SYMB_TIMEOUT_LSB            (0x1F)
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
uint32_t sx127x_rx_timeout_symb_to_ms(LORA* lora, uint32_t timeout_symb)
{
    uint32_t bw, sf, r_s, t_s, timeout_ms;
    const uint32_t domain_us = 1000000;//to keep precision with integer arith.

    bw = get_modem_config_signal_bandwidth_hz(lora);
    sf = lora->usr_config.spreading_factor;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * domain_us) / r_s;
    timeout_ms = (timeout_symb * t_s) / 1000;
    return timeout_ms;    
}
//NOTE: depends on get_modem_config_signal_bandwidth_hz
uint32_t sx127x_rx_timeout_ms_to_symb(LORA* lora, uint32_t timeout_ms)
{
    uint32_t bw, sf, r_s, t_s, timeout_symb;
    const uint32_t domain_us = 1000000;//to keep precision with integer arith.

    bw = get_modem_config_signal_bandwidth_hz(lora);
    sf = lora->usr_config.spreading_factor;
    // symbol rate
    r_s = bw / (1 << sf);//122,0703125
    // symbol period
    t_s = (1 * domain_us) / r_s;//8192 us per symbol
    timeout_symb = (timeout_ms * 1000) / t_s;
    return timeout_symb;
}
//NOTE: unused (returns same value as config->rx_timeout_ms)
uint32_t sx127x_get_rx_timeout_ms_single_reception(LORA* lora)
{
    uint32_t timeout_symb, timeout_ms;
    timeout_symb = get_rx_timeout_symb(lora);
    timeout_ms = sx127x_rx_timeout_symb_to_ms(lora, timeout_symb);
    return timeout_ms;
}

#define REG_PREAMBLE_MSB                (0x20)
#define REG_PREAMBLE_LSB                (0x21)
static inline int set_preamble_length(LORA* lora)
{
    /* The transmitted preamble length may be changed by setting the registers
       RegPreambleMsb and RegPreambleLsb from 6 to 65535, yielding total preamble
       lengths of 6 + 4 to 65535 + 4 symbols. */
    if (LORA_PREAMBLE_LEN < 6 || LORA_PREAMBLE_LEN > 65535)
        return -1;
    /* Preamble length MSB, = PreambleLength + 4.25 Symbols
        See Section 4.1.1.6 for more details. */
    write_reg(lora, REG_PREAMBLE_MSB, (LORA_PREAMBLE_LEN >> 8) & 0xff);
    /* Preamble Length LSB */
    write_reg(lora, REG_PREAMBLE_LSB, LORA_PREAMBLE_LEN & 0xff);
    return 0;
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

#define REG_PAYLOAD_LENGTH              (0x22)
static inline int set_payload_length(LORA* lora, uint8_t payload_length)
{
    /* A 0 value is not permitted */
    if (payload_length == 0)
        return -1;
    write_reg(lora, REG_PAYLOAD_LENGTH, payload_length);
    return 0;
}
static inline uint8_t get_payload_length(LORA* lora)
{
    /* Payload length in bytes. The register needs to be set in implicit
        header mode for the expected packet length. A 0 value is not
        permitted */
    return read_reg(lora, REG_PAYLOAD_LENGTH);
}

#define REG_MAX_PAYLOAD_LENGTH          (0x23)
static inline void set_max_payload_length(LORA* lora, uint8_t max_payload_length)
{
    /* Maximum payload length; if header payload length exceeds
        value a header CRC error is generated. Allows filtering of packet
        with a bad size. */
    write_reg(lora, REG_MAX_PAYLOAD_LENGTH, max_payload_length);
}
static inline uint8_t get_max_payload_length(LORA* lora)
{
    /* Maximum payload length; if header payload length exceeds
        value a header CRC error is generated. Allows filtering of packet
        with a bad size. */
    return read_reg(lora, REG_MAX_PAYLOAD_LENGTH);  
}

#define REG_HOP_PERIOD                  (0x24)
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

#define REG_FIFO_RX_BYTE_ADDR           (0x25)
static inline uint8_t get_rx_databuffer_pointer_current_value(LORA* lora)
{
    /* Current value of RX databuffer pointer (address of last byte
        written by Lora receiver) */
    return read_reg(lora, REG_FIFO_RX_BYTE_ADDR);     
}

#define REG_FEI_MSB                     (0x28)
#define REG_FEI_MID                     (0x29)
#define REG_FEI_LSB                     (0x2A)
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
    if (lora->usr_config.chip == LORA_CHIP_SX1272) 
    {
        //original formula: f_err = ((uint32_t)f_err_tc * (1U << 24)) / f_xtal;
        //re-write due to potential overflow of uint32_t
        f_err = (1U << 24) / (f_xtal / 1000);
        f_err = (f_err_tc * f_err) / 1000;
    }
    else if (lora->usr_config.chip == LORA_CHIP_SX1276)
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

#define REG_RSSI_WIDEBAND               (0x2C)
static inline uint8_t get_rssi_wideband(LORA* lora)
{
    /* Wideband RSSI measurement used to locally generate a
        random number */
    return read_reg(lora, REG_RSSI_WIDEBAND); 
}

#define REG_DETECT_OPTIMIZE             (0x31)
static inline void set_lora_detection_optimize(LORA* lora, uint8_t detection_optimize)
{
    uint8_t reg;
    reg = read_reg(lora, REG_DETECT_OPTIMIZE);
    /* LoRa detection Optimize
        0x03 - SF7 to SF12
        0x05 - SF6 */
    reg = setbits(reg, 2, 3, detection_optimize);
    write_reg(lora, REG_DETECT_OPTIMIZE, reg);
}
static inline uint8_t get_lora_detection_optimize(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_DETECT_OPTIMIZE);
    /* LoRa detection Optimize
        0x03 - SF7 to SF12
        0x05 - SF6 */
    return (reg >> 0) & 7;    
}

#define REG_INVERT_I_Q                  (0x33)
static inline void set_invert_lora_i_and_q_signals(LORA* lora)
{
    uint8_t reg;
    reg = read_reg(lora, REG_INVERT_I_Q);
    /* Invert the LoRa I and Q signals
        0 - normal mode
        1 - I and Q signals are inverted */
    reg = setbits(reg, 6, 1, LORA_INVERT_IQ);
    write_reg(lora, REG_INVERT_I_Q, reg);    
}
static inline uint8_t get_invert_lora_i_and_q_signals(LORA* lora)
{
    uint8_t reg = read_reg(lora, REG_INVERT_I_Q);
    /* Invert the LoRa I and Q signals
        0 - normal mode
        1 - I and Q signals are inverted */
    return (reg >> 6) & 1;     
}


#define REG_DETECTION_THRESHOLD         (0x37)
static inline void set_lora_detection_threshold(LORA* lora, uint8_t detection_threshold)
{
    /* LoRa detection threshold
        0x0A - SF7 to SF12
        0x0C - SF6 */
    write_reg(lora, REG_DETECTION_THRESHOLD, detection_threshold);    
}
static inline uint8_t get_lora_detection_threshold(LORA* lora)
{
    /* LoRa detection threshold
        0x0A - SF7 to SF12
        0x0C - SF6 */
    return read_reg(lora, REG_DETECTION_THRESHOLD);    
}

#define REG_SYNC_WORD                   (0x39)
static inline void set_lora_sync_word(LORA* lora)
{
    /* LoRa Sync Word
        Value 0x34 is reserved for LoRaWAN networks */
    write_reg(lora, REG_SYNC_WORD, LORA_SYNC_WORD_SX127X); 
}
static inline uint8_t get_lora_sync_word(LORA* lora)
{
    /* LoRa Sync Word
        Value 0x34 is reserved for LoRaWAN networks */
    return read_reg(lora, REG_SYNC_WORD); 
}
static inline uint32_t sx127x_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}
static inline uint32_t sx127x_ceil(uint32_t exp, uint32_t fra)
{ 
    return (fra > 0) ? exp+1 : exp;
}
static inline uint32_t get_symbol_length_ms(LORA* lora)
{
    uint32_t bw, sf, r_s, t_s;
    const uint32_t domain_us = 1000000;//to keep precision with integer arith.

    bw = get_modem_config_signal_bandwidth_hz(lora);
    sf = lora->usr_config.spreading_factor;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * domain_us) / r_s; 
    return t_s / 1000;
}
static inline uint32_t get_pkt_time_on_air_ms(LORA* lora)
{
    uint32_t bw, sf, r_s, t_s, pl, ih, de, crc, cr, exp, fra, dividend, divisor;
    uint32_t n_preamble, n_payload, t_preamble, t_payload, t_packet_ms;
    const uint32_t domain_us = 1000000;//to keep precision with integer arith.

    bw = get_modem_config_signal_bandwidth_hz(lora);
    sf = lora->usr_config.spreading_factor;
    n_preamble = LORA_PREAMBLE_LEN;
    // symbol rate
    r_s = bw / (1 << sf);
    // symbol period
    t_s = (1 * domain_us) / r_s;
    // preamble duration
    t_preamble = (n_preamble + 4.25) * t_s;
    
    // -- where PL is the number of bytes of payload
    pl = get_payload_length(lora);
    // -- SF is the spreading factor
    // -- IH = 1 when implicit header mode is enabled and IH = 0 when explicit header mode is used
    ih = lora->usr_config.implicit_header_mode_on ? 1 : 0;
    // -- DE set to 1 indicates the use of the low data rate optimization, 0 when disabled
    de = lora->sys_config.low_data_rate_optimize ? 1 : 0;
    // -- CRC indicates the presence of the payload CRC = 1 when on 0 when off
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    /* In Explicit header mode: CRC recovered from the header */
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    if (ih == 1 || lora->tx_on)
    {   
        crc = lora->sys_config.rx_payload_crc_on ? 1 : 0;
    }
    else
    {
        crc = get_crc_info_from_rcvd_pkt_header(lora);
    }

    // -- CR is the programmed coding rate from 1 to 4.
    cr = LORA_CODING_RATE;

    dividend = (8*pl) - (4*sf) + 28 + (16*crc) - (20*ih);
    divisor  = (4 * (sf - (2*de)));

    exp = dividend / divisor;
    fra = ((dividend * 10) / divisor) - (exp * 10);//get 1st digit of fractional part

    // number of payload symbols
    n_payload = 8 + sx127x_max(sx127x_ceil(exp, fra) * (cr+4), 0);
    // payload duration
    t_payload = n_payload * t_s;
    // total packet time on air
    t_packet_ms = (t_preamble + t_payload) / 1000;
    return t_packet_ms;
}

#define SX1272_REG_PA_DAC                   (0x5A)
#define SX1276_REG_PA_DAC                   (0x4D)
static inline bool set_pa_dac(LORA* lora, uint8_t pa_dac)
{
    uint8_t reg_pa_dac_addr;
    switch(lora->usr_config.chip)
    {
    case LORA_CHIP_SX1272: reg_pa_dac_addr = SX1272_REG_PA_DAC; break;
    case LORA_CHIP_SX1276: reg_pa_dac_addr = SX1276_REG_PA_DAC; break;
    default: return false;
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
    else if (pa_dac == 0x07 && 
            ((lora->usr_config.chip == LORA_CHIP_SX1272 && get_tx_output_power_dbm(lora) == 15) ||
             (lora->usr_config.chip == LORA_CHIP_SX1276 && get_tx_output_power_dbm(lora) == 17)))
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
    switch(lora->usr_config.chip)
    {
    case LORA_CHIP_SX1272: reg_pa_dac_addr = SX1272_REG_PA_DAC; break;
    case LORA_CHIP_SX1276: reg_pa_dac_addr = SX1276_REG_PA_DAC; break;
    default: return 0xFF;
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


#endif //SX127X_H