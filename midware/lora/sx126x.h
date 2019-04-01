/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#ifndef SX126X_H
#define SX126X_H

#include "spi.h"
#include "stdio.h"
#include <string.h>


//after each command is issued get its status and compare with CS_RFU
#define CHECK_CMD_STATUS          (0)
//after each byte is transferred via spi get its status and compare with CS_RFU
#define CHECK_CMD_BYTE_STATUS     (0)

/*
 * Table 12-1: List of Registers
 */
//Differentiate the LoRa® signal for Public or Private Network
#define REG_LORA_SYNC_WORD_MSB                  (0x0740)      
#define REG_LORA_SYNC_WORD_MSB_RESET            (0x14  )  
//Set to 0x3444 for Public Network Set to 0x1424 for Private Network
#define REG_LORA_SYNC_WORD_LSB                  (0x0741)  
#define REG_LORA_SYNC_WORD_LSB_RESET            (0x24  )
//Can be used to get a 32-bit random number    
#define REG_RANDOM_NUMBER_GEN_0                 (0x0819)     
#define REG_RANDOM_NUMBER_GEN_1                 (0x081A)     
#define REG_RANDOM_NUMBER_GEN_2                 (0x081B)     
#define REG_RANDOM_NUMBER_GEN_3                 (0x081C)     
//Set the gain used in Rx mode: Rx Power Saving gain: 0x94 Rx Boosted gain: 0x96
#define REG_RX_GAIN                             (0x08AC)      
#define REG_RX_GAIN_RESET                       (0x94  )  
//Set the Over Current Protection level. The value is changed internally depending
//on the device selected. Default values are: SX1262: 0x38 (140 mA) SX1261: 0x18 (60 mA)
#define REG_OCP_CONFIGURATION                   (0x08E7)      
#define REG_OCP_CONFIGURATION_RESET             (0x18  )  
//Value of the trimming cap on XTA pin This register should only be changed while
//the radio is in STDBY_XOSC mode.
#define REG_XTA_TRIM                            (0x0911)      
#define REG_XTA_TRIM_RESET                      (0x05  )  
//Value of the trimming cap on XTB pin This register should only be changed while
//the radio is in STDBY_XOSC mode.
#define REG_XTB_TRIM                            (0x0912)      
#define REG_XTB_TRIM_RESET                      (0x05  ) 
//The address of the register holding frequency error indication
//NOTE: not documented??
#define REG_FREQ_ERROR                          (0x076B)
//The address of the register holding the payload size
//NOTE: not documented??
#define REG_PAYLOAD_LENGTH                      (0x0702)


typedef enum
{
    CS_RESET = 0,
    CS_STARTUP,
                           //Optional registers, backup regulator, RC64k oscillator, data RAM
    CS_SLEEP_COLD_START,   // 0: cold start
    CS_SLEEP_WARM_START,   // 1: warm start (device configuration in retention)
    CS_STDBY_RC,           //Top regulator (LDO), RC13M oscillator 
    CS_STDBY_XOSC,         //Top regulator (DC-DC or LDO), XOSC
    CS_FS,                 //All of the above + Frequency synthesizer at Tx frequency
    CS_TX,                 //Frequency synthesizer and transmitter, Modem
    CS_RX,                 //Frequency synthesizer and receiver, Modem
    CS_UNKNOWN = 0xFF
} chip_state_t;

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
    CS_UNKNOWN_ = 0xFF 
} cmd_status_t;

static inline const char* cmd_status_str (cmd_status_t cmd_status)  
{
    switch (cmd_status)
    {
        case CS_RESERVED:                   return "CS_RESERVED";
        case CS_RFU:                        return "CS_RFU";
        case CS_DATA_IS_AVAILABLE_TO_HOST:  return "CS_DATA_IS_AVAILABLE_TO_HOST";
        case CS_COMMAND_TIMEOUT:            return "CS_COMMAND_TIMEOUT";
        case CS_COMMAND_PROCESSING_ERROR:   return "CS_COMMAND_PROCESSING_ERROR";
        case CS_FAILURE_TO_EXECUTE_COMMAND: return "CS_FAILURE_TO_EXECUTE_COMMAND";
        case CS_COMMAND_TX_DONE:            return "CS_COMMAND_TX_DONE";
        default:                            return "CS_UNKNOWN_";
    }
    return "CS_UNKNOWN_";
}

static inline void busy_pin_wait_failed()
{
    //todo: how to write busy_pin_wait_failed() exception handler in a such a way to
    //      return control flow back to highest lora call function (i.e. open/
    //      tx_async/rx_async) without multiple "return false" through all the
    //      lora functions below?? (all of them depend on busy-pin checking...)
    return;
}

static inline void busy_pin_wait_common(LORA* lora, uint8_t value)
{
    uint32_t r, i, count;

    //todo: how to select right value? (depending on CPU freq.?? faster freq => faster the loop??)
    //empirical test: on 48 MHz CPU (kinetis MK22) max count was registered as 296
    //todo: redo empirical test to find max count value on 48 MHz CPU (kinetis MK22)
    count = 100000;

    for (r = 0; r < LORA_SX126X_WAIT_BUSY_RETRY_TIMES; ++r)
    {
        for (i = 0; i < count; ++i)
        {
            if (gpio_get_pin(lora->usr_config.busy_pin) == value)
            {
                return;//return true;
            }
            //sleep_us(1);//in this case change "count" to "count_us"
        }
    }
    busy_pin_wait_failed();
    //return false;
}

static inline void busy_pin_wait_high(LORA* lora)
{
    busy_pin_wait_common(lora, 1);
}

static inline void busy_pin_wait_low(LORA* lora)
{
    busy_pin_wait_common(lora, 0);
}

#define SPI_DEBUG  (0)

static inline void cmd_common(LORA* lora, uint8_t cmd, void* extra, uint32_t esize, void* data, uint32_t size, bool is_write)
{
    uint32_t offset;
    uint8_t* iodata;
    LORA_SPI* spi;
    
    if (lora->chip_state_cached == CS_SLEEP_COLD_START || lora->chip_state_cached == CS_SLEEP_WARM_START)
    {
        /* In Sleep mode, the BUSY pin is held high through a 20 kΩ resistor
           and the BUSY line will go low as soon as the radio leaves the Sleep
           mode. */
        busy_pin_wait_high(lora);
    }
    else
    {        
        /* The BUSY control line is used to indicate the status of the 
           internal state machine. When the BUSY line is held low, it 
           indicates that the internal state machine is in idle mode and
           that the radio is ready to accept a command from the host 
           controller. */
        busy_pin_wait_low(lora);
    }

    spi     = &lora->spi;
    iodata  = (uint8_t*)io_data(spi->io);
    *iodata = cmd;
    offset  = 1;
    if (extra) 
    {
        memcpy(iodata+offset, extra, esize); offset += esize;
    }
    memcpy(iodata+offset, data, size);  offset += size;
    spi->io->data_size = offset;
    
#if (SPI_DEBUG)
    //if (is_write)
    {
        printf("[sx126x] [info] spi sent: ");
        for (int i = 0; i < spi->io->data_size; ++i)
        {
            printf("%02x ", iodata[i]);
        }
        printf("\n");
    }
#endif

    gpio_reset_pin(lora->usr_config.spi_cs_pin);
#if defined(STM32)
    //todo: could not use spi_send/spi_write_sync => use spi_read_sync
    spi_read_sync(spi->port, spi->io, spi->io->data_size);
#elif defined (MK22)
    if (is_write) spi_write_sync(spi->port, spi->io);
    else          spi_read_sync (spi->port, spi->io, spi->io->data_size);
#else
#error "unknown target" 
#endif
    gpio_set_pin(lora->usr_config.spi_cs_pin);

#if (SPI_DEBUG)
    //if (is_write)
    {
        printf("[sx126x] [info] spi rcvd: ");
        for (int i = 0; i < spi->io->data_size; ++i)
        {
            printf("%02x ", iodata[i]);
        }
        printf("\n");
    }
#endif

    offset = 1;
    if (extra)  
    {
        memcpy(extra, iodata+offset, esize); offset += esize;        
    }
    if (!is_write)//do not overwrite user data
    {
        memcpy(data, iodata+offset, size);
    }

    if (is_write && cmd == 0x84)//sleep
    {
        /* In Sleep mode, the BUSY pin is held high through a 20 kΩ resistor
           and the BUSY line will go low as soon as the radio leaves the Sleep
           mode. */
        busy_pin_wait_high(lora);
    }
    else
    {
        /* The BUSY control line is used to indicate the status of the 
           internal state machine. When the BUSY line is held low, it 
           indicates that the internal state machine is in idle mode and
           that the radio is ready to accept a command from the host 
           controller. */
        busy_pin_wait_low(lora);
    }
}

#if (CHECK_CMD_STATUS)
static inline cmd_status_t get_cmd_status (LORA* lora);

//Get status via get_cmd_status (spi transaction) and check it
static inline void check_cmd_status(LORA* lora, uint8_t cmd, bool is_write)
{
    //in progress...
}
#endif
static inline void cmd_write_extra(LORA* lora, uint8_t cmd, void* extra, uint32_t esize, void* data, uint32_t size)
{
    cmd_common(lora, cmd, extra, esize, data, size, true);
#if (CHECK_CMD_STATUS)
    check_cmd_status(lora, cmd, true);
#endif
}
static inline void cmd_read_extra(LORA* lora, uint8_t cmd, void* extra, uint32_t esize, void* data, uint32_t size)
{
    cmd_common(lora, cmd, extra, esize, data, size, false);
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

/* Table 9-1: SX1261/2 Operating Modes */
typedef enum 
{
    //Optional registers, backup regulator, RC64k oscillator, data RAM
    SLEEP = 0,
    //Top regulator (LDO), RC13M oscillator 
    STDBY_RC,
    //Top regulator (DC-DC or LDO), XOSC
    STDBY_XOSC,
    //All of the above + Frequency synthesizer at Tx frequency
    FS, 
    //Frequency synthesizer and transmitter, Modem
    TX, 
    //Frequency synthesizer and receiver, Modem
    RX,     
} op_mode_t;

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

/*
 * Table 11-5: Commands Returning the Radio Status
 */
//Returns the current status of the device
static inline void get_status (LORA* lora, uint8_t* status)  
{
    cmd_read(lora, 0xC0, status, sizeof(*status));
}
static inline chip_mode_t get_chip_mode_ext (uint8_t status)
{
    return (status >> 4) & 7;
}
static inline cmd_status_t get_cmd_status_ext (uint8_t status)
{
    return (status >> 1) & 7;
}
static inline chip_mode_t get_chip_mode (LORA* lora)
{
    uint8_t status;
    get_status (lora, &status);
    return get_chip_mode_ext(status);
}
static inline cmd_status_t get_cmd_status (LORA* lora)
{
    uint8_t status;
    get_status (lora, &status);
    return get_cmd_status_ext(status);
}

/* Table 13-85: OpError Bits */
typedef enum 
{
    OE_RC64K_CALIB_ERR = 0,/*bit 0   */ //RC64k calibration failed
    OE_RC13M_CALIB_ERR,    /*bit 1   */ //RC13M calibration failed
    OE_PLL_CALIB_ERR,      /*bit 2   */ //PLL calibration failed
    OE_ADC_CALIB_ERR,      /*bit 3   */ //ADC calibration failed
    OE_IMG_CALIB_ERR,      /*bit 4   */ //IMG calibration failed
    OE_XOSC_START_ERR,     /*bit 5   */ //XOSC failed to start
    OE_PLL_LOCK_ERR,       /*bit 6   */ //PLL failed to lock
    OE_RFU,                /*bit 7   */ 
    OE_PA_RAMP_ERR,        /*bit 8   */ //PA ramping failed
    OE_RFU_15_9            /*bit 15:9*/ 
} op_error_t;

//Returns the error which has occurred in the device
static inline void get_device_errors (LORA* lora, uint16_t* errors) 
{
    uint8_t extra[] = {0};//NOP
    uint8_t args[] = {0, 0};
    cmd_read_extra(lora, 0x17, extra, sizeof(extra), args, sizeof(args));
    *errors = 0;
    *errors |= args[0] << 8;
    *errors |= args[1];
}
//Returns true if op_error set
static inline bool get_device_error (LORA* lora, op_error_t op_error) 
{
    uint16_t errors;
    get_device_errors(lora, &errors);
    return (errors >> op_error) & 1;
}   
//Clear all the error(s). The error(s) cannot be cleared independently
static inline void clear_device_errors (LORA* lora)       
{
    uint8_t args[] = {0};
    cmd_write(lora, 0x07, args, sizeof(args));
}

static inline chip_state_t get_state_cur (LORA* lora)
{
    switch (lora->chip_state_cached)//get cached value to avoid spi trans
    {
        case CS_SLEEP_COLD_START:   return CS_SLEEP_COLD_START;
        case CS_SLEEP_WARM_START:   return CS_SLEEP_WARM_START;
        default: break;
    }
    switch (get_chip_mode(lora))
    {
        case CM_STBY_RC:   return CS_STDBY_RC;
        case CM_STBY_XOSC: return CS_STDBY_XOSC;
        case CM_FS:        return CS_FS;
        case CM_TX:        return CS_TX;
        case CM_RX:        return CS_RX;
        case CM_UNUSED:    
        case CM_RFU:       
        default:           return CS_UNKNOWN;
    }
    return CS_UNKNOWN;
}


static inline const char* chip_state_str (chip_state_t chip_state)
{
    switch (chip_state)
    {
        case CS_RESET:             return "CS_RESET";
        case CS_STARTUP:           return "CS_STARTUP";   
        case CS_SLEEP_COLD_START:  return "CS_SLEEP_COLD_START";           
        case CS_SLEEP_WARM_START:  return "CS_SLEEP_WARM_START";           
        case CS_STDBY_RC:          return "CS_STDBY_RC";   
        case CS_STDBY_XOSC:        return "CS_STDBY_XOSC";           
        case CS_FS:                return "CS_FS";           
        case CS_TX:                return "CS_TX";           
        case CS_RX:                return "CS_RX";           
        default:                   return "CS_UNKNOWN";   
    }
}

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
//Set Chip in SLEEP mode
/* The command SetSleep(...) is used to set the device in SLEEP mode 
with the lowest current consumption possible. This command can be 
sent only while in STDBY mode (STDBY_RC or STDBY_XOSC). After the 
rising edge of NSS, all blocks are switched OFF except the backup 
regulator if needed and the blocks specified in the parameter 
sleepConfig. */
static inline void set_sleep_ext (LORA* lora, sleep_config_t sleep_config)
{
    cmd_write(lora, 0x84, &sleep_config, sizeof(sleep_config));

    /* Caution:
    Once sending the command SetSleep(...), the device will become 
    unresponsive for around 500 us, time needed for the configuration 
    saving process and proper switch off of the various blocks. The user 
    must thus make sure the device will not be receiving SPI command 
    during these 500 us to ensure proper operations of the device. */
    sleep_us(500);
    
    /* When the radio is in Sleep mode, the BUSY pin is held high. */
    while (gpio_get_pin(lora->usr_config.busy_pin) == 0);
}

static inline void set_state (LORA* lora, chip_state_t state_new, ...);

static inline void set_sleep (LORA* lora, bool is_warm, chip_state_t state)
{
    if (state != LORA_STDBY_TYPE)
    {
        set_state(lora, LORA_STDBY_TYPE);
    }
    /* 9.3 Sleep Mode
       In this mode, most of the radio internal blocks are powered down or in
       low power mode and optionally the RC64k clock  and the timer are 
       running. The chip may enter this mode from STDBY_RC and can leave the 
       SLEEP mode if one of the following events occurs:
       - NSS pin goes low in any case
       - RTC timer generates an End-Of-Count (corresponding to Listen mode)
       When the radio is in Sleep mode, the BUSY pin is held high. */
    // RESERVED
    sleep_config_t sleep_config;
    sleep_config.SleepConfig_7_3 = 0;
    // 0: cold start, 
    // 1: warm start (device configuration in retention)
    sleep_config.SleepConfig_2   = is_warm ? 1 : 0;
    // 0: RFU
    sleep_config.SleepConfig_1   = 0;
    // 0: RTC timeout disable
    // 1: wake-up on RTC timeout (RC64k)
    sleep_config.SleepConfig_0   = 0;
    set_sleep_ext (lora, sleep_config);
}

static inline void set_sleep_warm_start (LORA* lora, chip_state_t state)
{
    set_sleep(lora, true, state);
}
static inline void set_sleep_cold_start (LORA* lora, chip_state_t state)
{
    set_sleep(lora, false, state);
}

/* Table 13-4: STDBY Mode Configuration */
typedef enum 
{
    /* Device running on RC13M, set STDBY_RC mode
       NOTE 0: RC13M - 13 MHz Resistance-Capacitance Oscillator
       NOTE 1: The 13 MHz RC oscillator (RC13M) is enabled for all SPI communication
       to permit configuration of the device without the need to start the crystal
       oscillator. */
    CONF_STDBY_RC   = 0,   
    /* Device running on XTAL 32MHz, set STDBY_XOSC mode */
    CONF_STDBY_XOSC = 1
} standby_config_t;

//Set Chip in STDBY_RC or STDBY_XOSC mode
static inline void set_standby (LORA* lora, standby_config_t standby_config)
{
    cmd_write(lora, 0x80, &standby_config, sizeof(standby_config));
}
//Set Chip in Freqency Synthesis mode
static inline void set_fs (LORA* lora)
{
    cmd_write(lora, 0xC1, 0, 0);
}

//todo: make timeout_23_0 more precise (for 5 ms expected 320, got 300...)
uint32_t get_timeout_23_0(uint32_t timeout_us)
{
    uint32_t timeout_23_0, div, mul;
    #define SX126X_TIMEOUT_MAX_VAL     (0xFFFFFF)
    #define SX126X_TIMEOUT_MAX_US      (262*1000*1000)
    #define SX126X_TIMEOUT_MIN_US      (16)   //round-up 15.625 us

    /* The maximum timeout is then 262 s */
    if (timeout_us > SX126X_TIMEOUT_MAX_US)
    {
#if (LORA_DEBUG)
        printf("[sx126x] [warning] timeout_us exceeds max value %d (set to max)\n", SX126X_TIMEOUT_MAX_US);
#endif
        timeout_us = SX126X_TIMEOUT_MAX_US;
    }
    else if (timeout_us < SX126X_TIMEOUT_MIN_US)
    {
#if (LORA_DEBUG)
        printf("[sx126x] [warning] timeout_us exceeds min value %d (set to min)\n", SX126X_TIMEOUT_MIN_US);
#endif
        timeout_us = SX126X_TIMEOUT_MIN_US;   
    }
    /* The timeout duration can be computed with the formula:
       Timeout duration = Timeout * 15.625 us */
    div = mul = 1;
         if (timeout_us >= 15625) { div = 15625; mul = 1000; }
    else if (timeout_us >= 1562 ) { div = 1562;  mul = 100; }
    else if (timeout_us >= 156  ) { div = 156;   mul = 10; }
    else if (timeout_us >= 16   ) { div = 16;    mul = 1; }
    timeout_23_0 = (timeout_us / div) * mul;
    return timeout_23_0; 
}

//Set Chip in Tx mode
static inline void set_tx (LORA* lora, uint32_t timeout_ms)
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
    cmd_write(lora, 0x83, args, sizeof(args));
}
//Set Chip in Rx mode
static inline void set_rx (LORA* lora, uint32_t timeout_ms)
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
    else if (timeout_ms == SX126X_TIMEOUT_MAX_VAL)
        timeout_23_0 = SX126X_TIMEOUT_MAX_VAL;
    else
        timeout_23_0 = get_timeout_23_0(timeout_ms*1000);

    uint8_t args[] = {
        (timeout_23_0 >> 16) & 0xff, 
        (timeout_23_0 >> 8 ) & 0xff, 
        (timeout_23_0 >> 0 ) & 0xff
    };
    cmd_write(lora, 0x82, args, sizeof(args));
}

static inline void set_reset (LORA* lora)
{
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    /* A complete “factory reset” of the chip can be issued on 
       request by toggling pin 15 NRESET of the SX1261/2. It will be
       automatically followed by the standard calibration procedure 
       and any previous context will be lost. The pin should be held
       low for more than 50 μs (typically 100 μs) for the Reset to 
       happen. */
    sleep_ms(20);
    gpio_reset_pin(usr_config->reset_pin);
    sleep_ms(50); 
    gpio_set_pin  (usr_config->reset_pin); 
    sleep_ms(20); 
}

static inline void set_startup (LORA* lora)
{
    uint32_t busy_cnt = 0;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    /* 9.1 Startup
       At power-up or after a reset, the chip goes into STARTUP state, the 
       control of the chip being done by the sleep state machine operating at
       the battery voltage. The BUSY pin is set to high indicating that the 
       chip is busy and cannot accept a command. When the digital voltage and
       RC clock become available, the chip can boot up and the CPU takes 
       control. At this stage the BUSY line goes down and the device is ready
       to accept commands. */
    while (gpio_get_pin(usr_config->busy_pin)) ++busy_cnt;
}

#if (LORA_DEBUG) && (LORA_DEBUG_STATES)
static inline void print_state_cur (LORA* lora)
{
    chip_state_t chip_state = get_state_cur(lora);
    printf("[sx126x] [info] state cur %u\n", chip_state);
}
#endif

static inline void set_state (LORA* lora, chip_state_t state_new, ...)
{
    chip_state_t state_cur;
    va_list va;
    int32_t timeout_ms;

    switch (state_new)
    {
        case CS_RESET:
            set_reset (lora);
            lora->chip_state_cached = CS_RESET;
            return;
        case CS_STARTUP:
            return;//unsupported, internal state
        default: break;
    }

    state_cur = get_state_cur(lora);

    if (state_cur == state_new)
    {
        //update in case of internal transitions (ex: CS_RESET->CS_STARTUP->CS_STDBY_RC)
        if (lora->chip_state_cached != state_cur)
            lora->chip_state_cached = state_cur;
        return;
    }

    switch (state_new)
    {
        //case CS_RESET:
        //    set_reset (lora);
        //    break;
        //case CS_STARTUP:
        //    set_startup (lora);
        //    break;
        case CS_SLEEP_COLD_START:
            set_sleep_cold_start(lora, state_cur);
            break;
        case CS_SLEEP_WARM_START:
            set_sleep_warm_start(lora, state_cur);
            break;
        case CS_STDBY_RC:
            set_standby (lora, CONF_STDBY_RC);
            break;                  
        case CS_STDBY_XOSC:  
            set_standby (lora, CONF_STDBY_XOSC);
            break;    
        case CS_FS:
            break;             
        case CS_TX:
            va_start(va, state_new);
            timeout_ms = va_arg(va, uint32_t);
            va_end(va);
            set_tx (lora, timeout_ms);
            break;
        case CS_RX:
            va_start(va, state_new);
            timeout_ms = va_arg(va, uint32_t);
            va_end(va);
            set_rx (lora, timeout_ms);
            break;
        default:
            break;//error unsupported             
    }
    /* 
     * NOTE: Do not wait specific delay from Table 8-2: Switching Time
     *       due to wait busy in spi write/read
     * //sleep_ms(xxx);
     */
    lora->chip_state_cached = state_new;

#if (LORA_DEBUG) && (LORA_DEBUG_STATES)
    //any spi trans in sleep state will cause wakeup => avoid these states
    if (state_new != CS_SLEEP_COLD_START && state_new != CS_SLEEP_WARM_START)
    {
        cmd_status_t cmd_status = get_cmd_status(lora);
        printf("[sx126x] [info] cmd_status %u\n", cmd_status);
    }
    print_state_cur(lora);
#endif
}

/* Table 13-11: StopOnPreambParam Definition */
typedef enum 
{
    /* Timer is stopped upon Sync Word or Header detection */
    STOP_ON_PREAMBLE_DISABLE = 0,
    /* Timer is stopped upon preamble detection */
    STOP_ON_PREAMBLE_ENABLE = 1,
} stop_on_preamble_param_t;
//Stop Rx timeout on Sync Word/Header or preamble detection
static inline void stop_timer_on_preamble (LORA* lora, stop_on_preamble_param_t stop_on_preamble_param)
{
    cmd_write(lora, 0x9F, &stop_on_preamble_param, sizeof(stop_on_preamble_param));
}
//Store values of RTC setup for listen mode and if period parameter is not 0, set chip into RX mode
static inline void set_rx_duty_cycle (LORA* lora, uint32_t rx_period_23_0, uint32_t sleep_period_23_0)
{
    uint8_t args[] = {
        (rx_period_23_0    >> 16) & 0xff, 
        (rx_period_23_0    >> 8 ) & 0xff, 
        (rx_period_23_0    >> 0 ) & 0xff,
        (sleep_period_23_0 >> 16) & 0xff, 
        (sleep_period_23_0 >> 8 ) & 0xff, 
        (sleep_period_23_0 >> 0 ) & 0xff,
    };
    cmd_write(lora, 0x94, args, sizeof(args));
}
//Set chip into RX mode with passed CAD parameters
static inline void set_cad (LORA* lora)
{
    cmd_write(lora, 0xC5, 0, 0);
}
//Set chip into TX mode with infinite carrier wave settings
static inline void set_tx_continuous_wave (LORA* lora)
{
    cmd_write(lora, 0xD1, 0, 0);
}
//Set chip into TX mode with infinite preamble settings
static inline void set_tx_infinite_preamble (LORA* lora)
{
    cmd_write(lora, 0xD2, 0, 0);
}
typedef enum 
{
    RM_LDO  = 0x00, // default
    RM_DCDC = 0x01,
} regulator_mode_t;
//Select LDO or DC_DC+LDO for CFG_XOSC, FS, RX or TX mode
static inline void set_regulator_mode (LORA* lora)
{
    regulator_mode_t mode = LORA_REGULATOR_MODE;
    cmd_write(lora, 0x96, &mode, sizeof(mode));
}
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
//Calibrate the RC13, RC64, ADC, PLL, Image according to parameter
static inline void calibrate (LORA* lora, calib_param_t calib_param)
{
    cmd_write(lora, 0x89, &calib_param, sizeof(calib_param));
}
//Launches an image calibration at the given frequencies
static inline void calibrate_image (LORA* lora, uint8_t freq_1, uint8_t freq_2 )
{
    uint8_t args[] = {freq_1, freq_2}; 
    cmd_write(lora, 0x98, args, sizeof(args));
}
//Configure the Duty Cycle, Max output power, device for the PA for SX1261 or SX1262
static inline void set_pa_config (LORA* lora, uint8_t pa_duty_cycle, uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut)
{
    uint8_t args[] = {pa_duty_cycle, hp_max, device_sel, pa_lut}; 
    cmd_write(lora, 0x95, args, sizeof(args));
}
typedef struct 
{
    
} fallback_mode_t;
//Defines into which mode the chip goes after a TX / RX done.
static inline void set_rx_tx_fallback_mode (LORA* lora, fallback_mode_t fallback_mode)
{
    cmd_write(lora, 0x93, &fallback_mode, sizeof(fallback_mode));
}


/* 
 * Table 11-2: Commands to Access the Radio Registers and FIFO Buffer 
 */
//Write into one or several registers
static inline void write_reg_burst (LORA* lora, uint16_t address_15_0, uint8_t* data, uint32_t size) 
{
    uint8_t extra[] = {
        (address_15_0 >> 8) & 0xff, 
        (address_15_0 >> 0) & 0xff
    };
    cmd_write_extra(lora, 0x0D, extra, sizeof(extra), data, size);
}
//Read one or several registers   
static inline void read_reg_burst (LORA* lora, uint16_t address_15_0, uint8_t* data, uint32_t size)
{
    uint8_t extra[] = {
        (address_15_0 >> 8) & 0xff, 
        (address_15_0 >> 0) & 0xff,
        /* Note that the host has to send an NOP after sending the 2 
           bytes of address to start receiving data bytes on the next 
           NOP sent. */
        0
    };
    cmd_read_extra(lora, 0x1D, extra, sizeof(extra), data, size);
}
static inline void write_reg (LORA* lora, uint16_t address_15_0, uint8_t data) 
{
    write_reg_burst(lora, address_15_0, &data, 1);
}
static inline void read_reg (LORA* lora, uint16_t address_15_0, uint8_t* data)
{
    read_reg_burst(lora, address_15_0, data, 1);
}  
//Write data into the FIFO
static inline void write_buffer (LORA* lora, uint8_t offset, uint8_t* data, uint32_t size) 
{
    uint8_t extra[] = {offset};
    cmd_write_extra(lora, 0x0E, extra, sizeof(extra), data, size);
}  
//Read data from the FIFO
static inline void read_buffer (LORA* lora, uint8_t offset, uint8_t* data, uint32_t size)
{
    uint8_t extra[] = {offset, 0};
    cmd_read_extra(lora, 0x1E, extra, sizeof(extra), data, size);
}

/*
 * Table 11-3: Commands Controlling the Radio IRQs and DIOs 
 */
//Configure the IRQ and the DIOs attached to each IRQ
static inline void set_dio_irq_params (LORA* lora, uint16_t irq_mask_15_0, uint16_t dio1_mask_15_0, 
    uint16_t dio2_mask_15_0, uint16_t dio3_mask_15_0) 
{
    uint8_t args[] = {
        (irq_mask_15_0  >> 8) & 0xff, 
        (irq_mask_15_0  >> 0) & 0xff,
        (dio1_mask_15_0 >> 8) & 0xff, 
        (dio1_mask_15_0 >> 0) & 0xff,
        (dio2_mask_15_0 >> 8) & 0xff, 
        (dio2_mask_15_0 >> 0) & 0xff,
        (dio3_mask_15_0 >> 8) & 0xff, 
        (dio3_mask_15_0 >> 0) & 0xff,
    };
    cmd_write(lora, 0x08, args, sizeof(args));
}
/* Table 8-4: IRQ Status Registers
Bit IRQ Description Protocol
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
    IRQ_RX_DONE,            //1   
    IRQ_PREAMBLE_DETECTED,  //2   
    IRQ_SYNC_WORD_VALID,    //3   
    IRQ_HEADER_VALID,       //4   
    IRQ_HEADER_ERR,         //5   
    IRQ_CRC_ERR,            //6   
    IRQ_CAD_DONE,           //7   
    IRQ_CAD_DETECTED,       //8   
    IRQ_TIMEOUT,            //9    
} irq_t;
//Get the values of the triggered IRQs 
static inline void get_irq_status (LORA* lora, /*uint8_t status,*/ uint16_t* irq_status_15_0) 
{
    uint8_t extra[] = {0};
    uint8_t args[] = {0, 0};
    cmd_read_extra(lora, 0x12, extra, sizeof(extra), args, sizeof(args));
    *irq_status_15_0 = 0;
    *irq_status_15_0 |= args[0] << 8;
    *irq_status_15_0 |= args[1];
    //*irq_status_15_0 &= 0x3FF;
}
static inline bool is_irq_set (LORA* lora, irq_t irq, uint16_t irq_status) 
{
    return (irq_status >> irq) & 1;
}
//Clear one or several of the IRQs
static inline void clear_irq_status (LORA* lora, uint16_t clear_irq_param_15_0)  
{
    /* This function clears an IRQ flag in the IRQ register by setting to 1 
       the bit of ClearIrqParam corresponding to the same position as the IRQ
       flag to be cleared. As an example, if bit 0 of ClearIrqParam is set 
       to 1 then IRQ flag at bit 0 of IRQ register is cleared. */
    uint8_t args[] = {
        (clear_irq_param_15_0 >> 8) & 0xff, 
        (clear_irq_param_15_0 >> 0) & 0xff
    };
    cmd_write(lora, 0x02, args, sizeof(args));
}
static inline void clear_irq_status_all (LORA* lora)  
{
    clear_irq_status(lora, 0xFFFF);
}
static inline void clear_irq_status_irq (LORA* lora, irq_t irq)  
{
    uint16_t clear_irq_param_15_0 = 0;
    clear_irq_param_15_0 |= (1 << irq);
    clear_irq_status(lora, clear_irq_param_15_0);
}
//Configure radio to control an RF switch from DIO2
static inline void set_dio2_as_rf_switch_ctrl (LORA* lora, uint8_t enable) 
{
    cmd_write(lora, 0x9D, &enable, 1);
}

//Configure the radio to use a TCXO controlled by DIO3  
static inline bool set_dio3_as_tcxo_ctrl (LORA* lora)
{
    uint32_t timeout_23_0 = get_timeout_23_0(LORA_OSC_32MHZ_STABILIZE_TIMEOUT_MS*1000);
    uint8_t args[] = {
        LORA_TCXO_VOLTAGE,
        /* The time needed for the 32 MHz to appear and stabilize can be 
           controlled through the parameter timeout. */
        (timeout_23_0 >> 16) & 0xff, 
        (timeout_23_0 >> 8 ) & 0xff, 
        (timeout_23_0 >> 0 ) & 0xff,
    };
#if (LORA_DEBUG)
    //clear_device_errors(lora);
#endif
    cmd_write(lora, 0x97, args, sizeof(args));
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

/*
 * Table 11-4: Commands Controlling the RF and Packets Settings
 */
//Set the RF frequency of the radio
static inline void set_rf_frequency_mhz (LORA* lora, uint32_t rf_frequency_mhz)
{
    uint32_t rf_freq_31_0;

#if 0 // FPU
    #define FREQ_XTAL   (32000000)
    #define FREQ_DIV    (33554432)
    #define FREQ_STEP   (0.95367431640625) // FREQ_XTAL / FREQ_DIV
    rf_freq_31_0 = (uint32_t)((double)rf_frequency_mhz / (double)FREQ_STEP);
    rf_freq_31_0 *= 1000000;
#else
    rf_freq_31_0 = ((uint64_t)rf_frequency_mhz * 1000000000000000) / 953674316;
#endif
    uint8_t args[] = {
        (rf_freq_31_0 >> 24) & 0xff, 
        (rf_freq_31_0 >> 16) & 0xff, 
        (rf_freq_31_0 >> 8 ) & 0xff, 
        (rf_freq_31_0 >> 0 ) & 0xff
    };
    cmd_write(lora, 0x86, args, sizeof(args));
}

/* Table 13-38: PacketType Definition */
typedef enum 
{
    /* GFSK packet type */
    PACKET_TYPE_GFSK = 0,   
    /* LORA mode */
    PACKET_TYPE_LORA = 1
} packet_type_t;

//Select the packet type corresponding to the modem    
static inline void set_packet_type (LORA* lora, packet_type_t packet_type)
{
    cmd_write(lora, 0x8A, &packet_type, sizeof(packet_type));
}
//Get the current packet configuration for the device    
static inline void get_packet_type (LORA* lora, uint8_t* packet_type)   
{
    uint8_t extra[] = {0};      *packet_type = 0;
    cmd_read_extra(lora, 0x11, extra, sizeof(extra), packet_type, sizeof(*packet_type)); 
}
//Set output power and ramp time for the PA
static inline void set_tx_params (LORA* lora, int8_t power)
{
    //#define DEV_SX1262    (0)
    #define DEV_SX1261      (1)
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
    if (!lora->usr_config.enable_high_power_pa && (power >= -17 && power <= 14))
    {
        set_pa_config (lora, 0x04, 0, DEV_SX1261, 0x01);
    }
    else if (lora->usr_config.enable_high_power_pa && (power >= -9 && power <= 15))
    {
        set_pa_config (lora, 0x06, 0, DEV_SX1261, 0x01);
    }
    else
    {
#if (LORA_DEBUG)
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, power);
#endif
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    /* Set the Over Current Protection level. The value is changed 
       internally depending on the device selected. Default values 
       are: SX1262: 0x38 (140 mA) SX1261: 0x18 (60 mA) */
    write_reg(lora, REG_OCP_CONFIGURATION, 0x18);
    
    uint8_t args[] = {power, LORA_RAMP_TIME};
    cmd_write(lora, 0x8E, args, sizeof(args));
}
//Compute and set values in selected protocol modem for given modulation parameters
static inline void set_modulation_params (LORA* lora, uint8_t mod_param1, uint8_t mod_param2, uint8_t mod_param3, uint8_t mod_param4_ld_opt)
{
    uint8_t args[] = {mod_param1, mod_param2, mod_param3, mod_param4_ld_opt};
    cmd_write(lora, 0x8B, args, sizeof(args)); 
}
//Set values on selected protocol modem for given packet parameters
static inline void set_packet_params (LORA* lora,
    uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4,uint8_t p5, uint8_t p6)
{
    uint8_t args[] = {p1, p2, p3, p4, p5, p6};
    cmd_write(lora, 0x8C, args, sizeof(args));
}
//NOTE: to set just payload_length need to re-set all params... => wrap into set_packet_params_ext
static inline void set_packet_params_ext(LORA* lora, uint8_t payload_length)
{
    LORA_SYS_CONFIG* sys_config = &lora->sys_config;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
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
    p[2] = usr_config->implicit_header_mode_on;
    /* Table 13-68: LoRa® PacketParam4 - PayloadLength
       Payloadlength   Description
       0x00 to 0xFF    Size of the payload (in bytes) to transmit or maximum size of the
                       payload that the receiver can accept. */
    p[3] = payload_length;
    /* Table 13-69: LoRa® PacketParam5 - CRCType
       CRCType Description
       0x00    CRC OFF
       0x01    CRC ON */
    p[4] = sys_config->rx_payload_crc_on;
    /* Table 13-70: LoRa® PacketParam6 - InvertIQ
       AddrComp    Description
       0x00        Standard IQ setup
       0x01        Inverted IQ setup */
    p[5] = LORA_INVERT_IQ;
    set_packet_params (lora, p[0], p[1], p[2], p[3], p[4], p[5]);
} 
static inline void set_payload_length(LORA* lora, uint8_t payload_length)
{
    set_packet_params_ext(lora, payload_length);
}
//Set the parameters which are used for performing a CAD (LoRa® only)
static inline void set_cad_params (LORA* lora, uint8_t cad_symbol_num,
    uint8_t cad_det_peak, uint8_t cad_det_min, uint8_t cad_exit_mode, uint8_t cad_timeout)
{
    uint8_t args[] = {cad_symbol_num, cad_det_peak, cad_det_min, cad_exit_mode, cad_timeout};
    cmd_write(lora, 0x88, args, sizeof(args));
}
//Store TX and RX base address in regis- ter of selected protocol modem   
static inline void set_buffer_base_address (LORA* lora, uint8_t tx_base_addr, uint8_t rx_base_addr) 
{
    uint8_t args[] = {tx_base_addr, rx_base_addr};
    cmd_write(lora, 0x8F, args, sizeof(args));
}
//Set the number of symbol the modem has to wait to validate a lock 
static inline void set_lora_symb_num_timeout (LORA* lora, uint8_t symb_num) 
{
    cmd_write(lora, 0xA0, &symb_num, sizeof(symb_num));
}
//Returns the instantaneous measured RSSI while in Rx mode
static inline void get_rssi_inst (LORA* lora, uint8_t* rssi)
{
    uint8_t extra[] = {0};//NOP
    cmd_read_extra(lora, 0x15, extra, sizeof(extra), rssi, sizeof(*rssi));
}    
//Returns PaylaodLengthRx(7:0), RxBufferPointer(7:0)
static inline void get_rx_buffer_status (LORA* lora, uint8_t* paylaod_length_rx_7_0, uint8_t* rx_buffer_pointer_7_0)
{
    uint8_t extra[] = {0};//NOP
    uint8_t args[] = {0, 0};
    cmd_read_extra(lora, 0x13, extra, sizeof(extra), args, sizeof(args));
    *paylaod_length_rx_7_0 = args[0];
    *rx_buffer_pointer_7_0 = args[1];
} 
/* Returns RssiPkt, SnrPkt, SignalRssiPkt 
   RssiPkt - Average over last packet received of RSSI (Actual signal power is –RssiPkt/2 (dBm))
   SnrPkt  - Estimation of SNR on last packet received.In two’s compliment format multiplied by 4. 
             (Actual SNR in dB =SnrPkt/4)
   SignalRssiPkt - Estimation of RSSI of the LoRa® signal (after despreading) on last packet received.
                   In two’s compliment format. (Actual Rssi in dB = -SignalRssiPkt/2) */
static inline void get_packet_status (LORA* lora, uint8_t* rssi_pkt, uint8_t* snr_pkt, uint8_t* signal_rssi_pkt)  
{
    uint8_t extra[] = {0};//NOP
    uint8_t args[] = {0, 0, 0};
    cmd_read_extra(lora, 0x14, extra, sizeof(extra), args, sizeof(args));
    *rssi_pkt        = args[0];
    *snr_pkt         = args[1];
    *signal_rssi_pkt = args[2];
}

//Returns statistics on the last few received packets
typedef struct 
{
    
} stats_t;
static inline void get_stats (LORA* lora, stats_t* stats)  
{
    cmd_read(lora, 0x10, stats, sizeof(*stats));
}  
//Resets the value read by the command GetStats
static inline void reset_stats (LORA* lora)  
{
    cmd_write(lora, 0x00, 0, 0);
}

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

static inline uint32_t get_bandwidth_hz(LORA* lora)
{
    switch (lora->usr_config.signal_bandwidth)
    {
        case LORA_BW_500: return 500000;
        case LORA_BW_250: return 250000;
        case LORA_BW_125: return 125000;
        case LORA_BW_062: return 62500;
        case LORA_BW_041: return 41670;
        case LORA_BW_031: return 31250;
        case LORA_BW_020: return 20830;
        case LORA_BW_015: return 15630;
        case LORA_BW_010: return 10420;
        case LORA_BW_007: return 7810;
        default: return 0;
    }
    return 0;
}

static inline uint8_t get_payload_length(LORA* lora)
{
    return lora->usr_config.payload_length;
}

static inline uint8_t get_crc_info_from_rcvd_pkt_header(LORA* lora)
{
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    //NOTE: could not find in the sx126x data sheet how to obtain CRC presence
    //      from header when explicit header mode is used
    //NOTE: in sx127x CrcOnPayload is used
    //Solution: in case of LORA_TX rx_payload_crc_on is set to 1 (see sx12xx_config)
    //          => CRC is always present in the header
    return 1;
}


#endif //SX126X_H