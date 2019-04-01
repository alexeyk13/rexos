/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#include "../userspace/gpio.h"
#include "loras_private.h"

#if defined (LORA_SX127X)

#include "sx127x.h" 

#define POR_RESET_MS   (10)   

char* lora_get_uptime_us(char* buf);

//#define SPI_TEST

#if defined(SPI_TEST)
void sx127x_spi_test(LORA* lora)
{
    const int test_sel = 0;
    uint8_t reg = 0, cnt = 0;

    if (test_sel == 0)
    {
        while (1)
        {
            reg = read_reg(lora, REG_OP_MODE);
            printf("[sx127x] [info] REG_OP_MODE 0x%02x cnt:%d\n", reg, cnt);
            sleep_ms(100);
            write_reg(lora, REG_OP_MODE, cnt);
            sleep_ms(100);
            if (cnt++ >= 7) cnt = 0; 
        }
    }
    else if (test_sel == 1) 
    {
        while (1)
        {
            reg = read_reg(lora, REG_IRQ_FLAGS);
            sleep_ms(10);
            printf("[sx127x] [info] REG_IRQ_FLAGS 0x%02x\n", reg);
        }
    }
    else if (test_sel == 2) 
    {
        while (1)
        {
            LORA_CHIP chip = get_version(lora);
            sleep_ms(100);
            printf("[sx127x] [info] chip %d\n", chip);
        }
    }
}
#endif

static void sx127x_reset(LORA* lora)
{
    SYSTIME uptime;
    uint32_t ms_passed, rem_por_delay_ms;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    if (lora->por_reset_completed)
    {
        /* 7.2.2. Manual Reset */
        if (LORA_DO_MANUAL_RESET_AT_OPEN)
        {
            if      (usr_config->chip == LORA_CHIP_SX1272) gpio_set_pin  (usr_config->reset_pin);
            else if (usr_config->chip == LORA_CHIP_SX1276) gpio_reset_pin(usr_config->reset_pin);
            sleep_ms(2); 
            if      (usr_config->chip == LORA_CHIP_SX1272) gpio_reset_pin(usr_config->reset_pin);
            else if (usr_config->chip == LORA_CHIP_SX1276) gpio_set_pin  (usr_config->reset_pin);
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
        lora->por_reset_completed = true;
    }
}

void sx127x_open(LORA* lora)
{
    mode_t mode;
    LORA_CHIP chip;
    LORA_SYS_CONFIG* sys_config = &lora->sys_config;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;

    sx127x_reset(lora);

#if defined(SPI_TEST)
    sx127x_spi_test(lora);
#endif
    
    /* check for version */
    chip = get_version(lora);
    if (chip != usr_config->chip)
    {
#if (LORA_DEBUG)
        lora_set_config_error(lora, __FILE__, __LINE__, 1, chip);
#endif
        error(ERROR_NOT_CONFIGURED);
        return;
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
    switch (usr_config->chip)
    {
        case LORA_CHIP_SX1272: 
            set_rf_carrier_frequency(lora, usr_config->rf_carrier_frequency_mhz);
        break;
        case LORA_CHIP_SX1276: 
            /* Access Low Frequency Mode registers
                0 - High Frequency Mode (access to HF test registers)
                1 - Low Frequency Mode (access to LF test registers) */
            /* The registers in the address space from 0x61 to 0x73 are specific
               for operation in the lower frequency bands (below 525 MHz), or in
               the upper frequency bands (above 779 MHz). */
            /* NOTE: For SX1272: 860 - 1020 MHz (Table 7)
               NOTE: For SX1276:     
                     Band 1 (HF) 862 - 1020 MHz (Table 32)
                     Band 2 (LF) 410 - 525  MHz (Table 32)
                     Band 3 (LF) 137 - 175  MHz (Table 32) */
            if      (usr_config->rf_carrier_frequency_mhz <= 525) 
                set_low_frequency_mode_on(lora, 1);
            else if (usr_config->rf_carrier_frequency_mhz >= 862) 
                set_low_frequency_mode_on(lora, 0);

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
            set_frequency_additional_registers(lora, 
                sys_config->agc_reference_level,
                sys_config->agc_step1, sys_config->agc_step2,
                sys_config->agc_step3, sys_config->agc_step4,
                sys_config->agc_step5, sys_config->pll_bandwidth);
            break;
        default: break;
    }


    if (usr_config->enable_high_power_pa)
    {
        /* [pa_select] Selects PA output pin:
                0 RFIO pin. Output power is limited to 13 dBm.
                1 PA_BOOST pin. Output power is limited to 20 dBm */
        sys_config->pa_select = 1;
        /* Enables the +20 dBm option on PA_BOOST pin
           0x04 - Default value
           0x07 - +20 dBm on PA_BOOST when OutputPower = 1111 */
        /* Notes:
            - High Power settings must be turned off when using PA0 
            - The Over Current Protection limit should be adapted to
              the actual power level, in RegOcp */                  
        //todo: HOW to adapt the OCP to the actual power level according
        //      to actual power level??
        //note: in Table 35 "Trimming of the OCP" OcpTrim values are
        //      given only for I_MAX (mA) (but no actual power level)
        if (set_pa_dac_high_power_20_dBm(lora) == false)
        {
#if (LORA_DEBUG)
            lora_set_intern_error(lora, __FILE__, __LINE__, 0);
#endif
            error(ERROR_NOT_CONFIGURED);
            return; 
        }
    }

    if (usr_config->chip == LORA_CHIP_SX1272) 
    {
        /* [pa_select] Selects PA output pin:
                        0 RFIO pin. Output power is limited to 13 dBm.
                        1 PA_BOOST pin. Output power is limited to 20 dBm */
        /* [tx_output_power_dbm] Power amplifier max output power:
                        Pout = 2 + OutputPower(3:0) on PA_BOOST.
                        Pout = -1 + OutputPower(3:0) on RFIO. */
        set_pa_sx1272(lora, sys_config->pa_select, sys_config->tx_output_power_dbm);
    }
    else if (usr_config->chip == LORA_CHIP_SX1276) 
    {
        /* [pa_select] Selects PA output pin
                0 - RFO pin. Output power is limited to +14 dBm.
                1 - PA_BOOST pin. Output power is limited to +20 dBm */
        /* [max_power] Select max output power: Pmax=10.8+0.6*MaxPower [dBm] */
        /* [tx_output_power_dbm] 
                Pout=Pmax-(15-OutputPower) if PaSelect = 0 (RFO pin)
                Pout=17-(15-OutputPower) if PaSelect = 1 (PA_BOOST pin) */
        set_pa_sx1276(lora, sys_config->pa_select, sys_config->max_power, 
                sys_config->tx_output_power_dbm);
    }

    set_ocp(lora);

    set_lna(lora);

    /* [fifo_addr_ptr] SPI interface address pointer in FIFO data buffer. */
    /* [fifo_tx_base_addr] write base address in FIFO data buffer for TX modulator */  
    /* [fifo_rx_base_addr] read base address in FIFO data buffer for RX demodulator */  
    set_fifo_address_pointers(lora, sys_config->fifo_addr_ptr,
        sys_config->fifo_tx_base_addr, sys_config->fifo_rx_base_addr);

    // unmask all irqs
    set_reg_irq_flags_mask_all(lora);

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    //if (usr_config->chip == LORA_CHIP_SX1272) {
        /* [signal_bandwidth] Signal bandwidth:
                00 - 125 kHz
                01 - 250 kHz
                10 - 500 kHz
                11 - reserved */ 
    //else if (usr_config->chip == LORA_CHIP_SX1276) {
        /* [signal_bandwidth] Signal bandwidth:
                0000 - 7.8 kHz    0001 - 10.4 kHz
                0010 - 15.6 kHz   0011 - 20.8kHz
                0100 - 31.25 kHz  0101 - 41.7 kHz
                0110 - 62.5 kHz   0111 - 125 kHz
                1000 - 250 kHz    1001 - 500 kHz
                other values - reserved
                In the lower band (169MHz), signal bandwidths 8&9 are not supported) */
    //}
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    /* [rx_payload_crc_on] Enable CRC generation and check on payload:
            0 - CRC disable
            1 - CRC enable
            If CRC is needed, RxPayloadCrcOn should be set:
            - in Implicit header mode: on Tx and Rx side
            - in Explicit header mode: on the Tx side alone (recovered from
            the header in Rx side) */
    /* The corresponding bit (RxPayloadCrcOn) is hence unused on the receive side. */
    if (usr_config->implicit_header_mode_on == 0)
    {
        if (usr_config->txrx == LORA_RX)
            sys_config->rx_payload_crc_on = 0;
        else if (usr_config->txrx == LORA_TXRX)
            sys_config->rx_payload_crc_on = 1;

#if (LORA_DEBUG) && (LORA_DEBUG_INFO)
        printf("[sx127x] [info] %s in Explicit Header mode: rx_payload_crc_on %s\n", 
            usr_config->txrx == LORA_TX ? "TX" :
            usr_config->txrx == LORA_RX ? "RX" : 
            usr_config->txrx == LORA_TXRX ? "TX-RX" : "NA",
            sys_config->rx_payload_crc_on ? "ENABLED" : "DISABLED"); 
#endif
    }
    if (usr_config->chip == LORA_CHIP_SX1276) 
    {
        /* [low_data_rate_optimize] LowDataRateOptimize:
                0 - Disabled
                1 - Enabled; mandated for when the symbol length exceeds 16 ms */
        //NOTE: is "symbol length" same as "symbol period" (t_s)??
        if (get_symbol_length_ms(lora) > 16 && sys_config->low_data_rate_optimize == 0)
        {
#if (LORA_DEBUG)
            lora_set_config_error(lora, __FILE__, __LINE__, 0);
#endif
            return;
        }
    }
    /* [spreading_factor] SF rate (expressed as a base-2 logarithm)
             6 -   64 chips / symbol
             7 -  128 chips / symbol
             8 -  256 chips / symbol
             9 -  512 chips / symbol
            10 - 1024 chips / symbol
            11 - 2048 chips / symbol
            12 - 4096 chips / symbol
            other values reserved. */ 
    set_modem_config(lora,
        usr_config->signal_bandwidth,            
        usr_config->implicit_header_mode_on,     sys_config->rx_payload_crc_on,
        sys_config->low_data_rate_optimize,      usr_config->spreading_factor);

    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)  
    {  
        /* In this mode, the modem searches for a preamble during a given period 
           of time. If a preamble hasn’t been found at the end of the time 
           window, the chip generates the RxTimeout interrupt and goes back to 
           Standby mode. The length of the reception window (in symbols) is 
           defined by the RegSymbTimeout register and should be in the range of 
           4 (minimum time for the modem to acquire lock on a preamble) up to 
           1023 symbols. */ 
        /* [symb_timeout] RX operation time-out value expressed as number of symbols */
        uint32_t rx_timeout_symb = sx127x_rx_timeout_ms_to_symb(lora, usr_config->rx_timeout_ms);
        if (rx_timeout_symb < 4) 
        {
#if (LORA_DEBUG)
            lora_set_config_error(lora, __FILE__, __LINE__, 0);
#endif
            return;
        }
        else if (rx_timeout_symb > 1023)
        {
#if (LORA_DEBUG)
            lora_set_config_error(lora, __FILE__, __LINE__, 0);
#endif
            return;
        }
        set_rx_timeout(lora, rx_timeout_symb); 
    }

    /* The transmitted preamble length may be changed by setting
       the register RegPreambleMsb and RegPreambleLsb from 6 to 65535, yielding
       total preamble lengths of 6 + 4 to 65535 + 4 symbols.
       Preamble length MSB+LSB, = PreambleLength + 4.25 Symbols
       See Section 4.1.1.6 for more details. */
    set_preamble_length(lora);

    /* [payload_length] Payload length in bytes. The register needs to be set in 
            implicit header mode for the expected packet length. A 0 value is not
            permitted */
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    if (usr_config->implicit_header_mode_on == 1 && lora->rx_on)
    {
        //NOTE: need pl for time_on_air_ms calc. (to check if timer ms values are reasonable)
        set_payload_length(lora, usr_config->payload_length);
    }
    else
    {
        /* Otherwise, in explicit header mode, the initial size of the receive
           buffer is set to the packet length in the received header. */
        //NOTE: need pl for time_on_air_ms calc. (to check if timer ms values are reasonable)
        set_payload_length(lora, usr_config->payload_length);
    }
    
    /* [max_payload_length] Maximum payload length; if header payload
            length exceeds value a header CRC error is generated. Allows
            filtering of packet with a bad size. */
    set_max_payload_length(lora, usr_config->max_payload_length);

    set_symbol_periods_between_frequency_hops(lora);


    if (usr_config->chip == LORA_CHIP_SX1276)
    {
        set_ppm_correction(lora);
    }

    /* [detection_optimize] LoRa detection Optimize
            0x03 - SF7 to SF12
            0x05 - SF6 */
    set_lora_detection_optimize(lora, sys_config->detection_optimize);

    /* Invert the LoRa I and Q signals
        0 - normal mode
        1 - I and Q signals are inverted */
    set_invert_lora_i_and_q_signals(lora);

    /* [detection_threshold] LoRa detection threshold
            0x0A - SF7 to SF12
            0x0C - SF6 */
    set_lora_detection_threshold(lora, sys_config->detection_threshold);

    set_lora_sync_word(lora);
}

void sx127x_close(LORA* lora)
{
    set_mode(lora, SLEEP);
}

void sx127x_tx_async_wait(LORA* lora)
{
#if (LORA_DEBUG)
    mode_t mode;
#endif
    uint8_t irq_flags;
 
#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;
    }
    //NOTE: Do not know if packet is already sent (state is TX or STDBY)
    //      => do not check mode
#endif    

    // wait for tx_done IRQ
    irq_flags = get_reg_irq_flags_all(lora);
    if (get_reg_irq_flags(lora, IRQ_TX_DONE, irq_flags))
    {
#if (LORA_DEBUG) && (LORA_DEBUG_IRQS)
        printf("[sx127x] [info] got irq 0x%02x (TX_DONE) wait_irq_cnt %d\n", irq_flags, wait_irq_cnt);
#endif
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else
    {
        return;//no irq
    }

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_DATA)
    if (usr_config->txrx == LORA_TX || usr_config->txrx == LORA_RX)
    {
        uint8_t fifo_tx_base_addr = sys_config->fifo_tx_base_addr;
        uint8_t fifo_rx_base_addr = sys_config->fifo_rx_base_addr;
        uint8_t fifo_addr_ptr     = fifo_tx_base_addr;
        set_fifo_address_pointers(lora, fifo_addr_ptr, fifo_tx_base_addr, fifo_rx_base_addr);
        uint8_t buffer[32];
        uint8_t size = 32;
        read_fifo(lora, buffer, size);
        printf("[sx127x] [info] fifo after read [ ");
        for (int i = 0; i < size; ++i)
        {
            printf("%d ", buffer[i]);
        }
        printf("]\n");
    }
    #endif

    // Auto-go back to Standby mode
    /* When activated the SX1272/73 powers all remaining blocks required
       for transmit, ramps the PA, transmits the packet and returns to Standby mode */
#if (LORA_DEBUG)
    mode = get_mode(lora);
    if (mode != STDBY)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, mode);//EXTRA_STATE_CHECK
        return;
    }
#endif    
    // Go back from STDBY to SLEEP
    set_mode(lora, SLEEP);
}

void sx127x_tx_async(LORA* lora, IO* io)
{
#if (LORA_DEBUG)    
    mode_t mode;
#endif
    uint8_t fifo_tx_base_addr, fifo_rx_base_addr, fifo_addr_ptr;
#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_PTRS)
    uint8_t fifo_rx_current_addr;
#endif
    LORA_SYS_CONFIG* sys_config = &lora->sys_config;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    
#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_COMPLETED)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;    
    }
    mode = get_mode(lora);
    if (mode != SLEEP)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, mode);//EXTRA_STATE_CHECK
        return;
    }
#endif    

    /* See Figure 18. Startup Process */ 
    set_mode(lora, STDBY);

    // tx init
    /* LoRaTM Transmit Data FIFO Filling
        In order to write packet data into FIFO user should:
        1 Set FifoAddrPtr to FifoTxBaseAddrs.
        2 Write PayloadLength bytes to the FIFO (RegFifo) */
    fifo_tx_base_addr = sys_config->fifo_tx_base_addr;
    fifo_rx_base_addr = sys_config->fifo_rx_base_addr;
    fifo_addr_ptr     = fifo_tx_base_addr;
    set_fifo_address_pointers(lora, fifo_addr_ptr, fifo_tx_base_addr, fifo_rx_base_addr);

#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_PTRS)
    get_fifo_address_pointers(lora, &fifo_addr_ptr, &fifo_tx_base_addr, &fifo_rx_base_addr, &fifo_rx_current_addr);
    printf("[sx127x] [info] ptr before write_fifo\n");
    printf("  fifo_addr_ptr           %02x\n", fifo_addr_ptr);
    printf("  fifo_tx_base_addr       %02x\n", fifo_tx_base_addr);
    printf("  fifo_rx_base_addr       %02x\n", fifo_rx_base_addr);
    printf("  fifo_rx_current_addr    %02x\n", fifo_rx_current_addr);
#endif

    // 2 Write PayloadLength bytes to the FIFO (RegFifo)
    write_fifo(lora, io_data(io), io->data_size);

#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_PTRS)
    get_fifo_address_pointers(lora, &fifo_addr_ptr, &fifo_tx_base_addr, &fifo_rx_base_addr, &fifo_rx_current_addr);
    printf("[sx127x] [info] ptr after write_fifo\n");
    printf("  fifo_addr_ptr           %02x\n", fifo_addr_ptr);
    printf("  fifo_tx_base_addr       %02x\n", fifo_tx_base_addr);
    printf("  fifo_rx_base_addr       %02x\n", fifo_rx_base_addr);
    printf("  fifo_rx_current_addr    %02x\n", fifo_rx_current_addr);
    printf("  get_payload_length      %d\n", get_payload_length(lora));
#endif

    // The register RegPayloadLength indicates the size of the memory location to be transmitted
    set_payload_length(lora, io->data_size);

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    // mode request - TX
    set_mode(lora, TX);

    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    lora->error = LORA_ERROR_NONE;

    if (usr_config->tx_timeout_ms > 0)
    {
        timer_start_ms(lora->timer_txrx_timeout, usr_config->tx_timeout_ms);
    }
}

void sx127x_rx_async_wait(LORA* lora)
{
#if (LORA_DEBUG)    
    mode_t mode;
#endif
    uint8_t irq_flags, can_read_fifo, payload_size, condition;
#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_PTRS)
    uint8_t fifo_addr_ptr, fifo_tx_base_addr, fifo_rx_base_addr;
#endif
    uint8_t fifo_rx_current_addr;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    
#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;    
    }
    mode = get_mode(lora);
    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
    {
        //When activated the SX1272/73 powers all remaining blocks required for reception, remains
        //in this state until a valid packet has been received and then returns to Standby mode.
        //NOTE: Do not know if packet is received (state RXSINGLE or STDBY)
        //      => do not check state
    }
    else if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION)
    {
        if (mode != RXCONTINUOUS)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, mode);//EXTRA_STATE_CHECK
            return;
        }  
    }
#endif

    /* 2 Upon reception of a valid header CRC the RxDone interrupt is set. The radio remains
        in RXCONT mode waiting for the next RX LoRaTM packet. */
    irq_flags = get_reg_irq_flags_all(lora);
    // wait for RX_DONE IRQ
    if (get_reg_irq_flags(lora, IRQ_RX_DONE, irq_flags))
    {
#if (LORA_DEBUG) && (LORA_DEBUG_IRQS)
        printf("[sx127x] [info] got irq 0x%02x (RX_DONE)\n", irq_flags);
#endif
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else if (usr_config->recv_mode == LORA_SINGLE_RECEPTION && get_reg_irq_flags(lora, IRQ_RX_TIMEOUT, irq_flags))
    {
        lora_set_error(lora, LORA_ERROR_RX_TIMEOUT);
        return;
    }
    else 
    {
        return;//no irq
    }

    /* 3 The PayloadCrcError flag should be checked for packet integrity. */
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    if (usr_config->implicit_header_mode_on == 1 || get_crc_info_from_rcvd_pkt_header(lora) == 1)
    {
        if (get_reg_irq_flags(lora, IRQ_PAYLOAD_CRC_ERROR, irq_flags))
        {
#if (LORA_DEBUG) && (LORA_DEBUG_IRQS)
            printf("[sx127x] [info] got irq 0x%02x (PAYLOAD_CRC_ERROR) wait_irq_cnt %d\n", irq_flags, wait_irq_cnt);
#endif
            lora_set_error(lora, LORA_ERROR_PAYLOAD_CRC_INCORRECT);
            return;
        }
    }
    else 
    {
        /* If the bit CrcOnPayload is at ‘0’, it means there was no CRC on the payload and thus the IRQ
            Flag PayloadCrcError will not be trigged in the event of payload errors. */
        lora_set_error(lora, LORA_ERROR_NO_CRC_ON_THE_PAYLOAD);
        return;
    }

    can_read_fifo = 1;
    /* 4 If packet has been correctly received the FIFO data buffer can be read (see below). */
    /*      In order to retrieve received data from FIFO the user must ensure that ValidHeader, PayloadCrcError,
            RxDone and RxTimeout interrupts in the status register RegIrqFlags are not asserted */
    /* NOTE:    In continuous RX mode, opposite to the single RX mode, the RxTimeout interrupt will never
                occur and the device will never go in Standby mode automatically. */
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    if (usr_config->implicit_header_mode_on == 1)
    {
        condition = get_reg_irq_flags(lora, IRQ_RX_DONE,           irq_flags) && 
                    get_reg_irq_flags(lora, IRQ_PAYLOAD_CRC_ERROR, irq_flags) == 0;
    }
    else
    {
        condition = get_reg_irq_flags(lora, IRQ_VALID_HEADER,      irq_flags) && 
                    get_reg_irq_flags(lora, IRQ_RX_DONE,           irq_flags) && 
                    get_reg_irq_flags(lora, IRQ_PAYLOAD_CRC_ERROR, irq_flags) == 0;
    }
    if (!condition)
    {
        /* In case of errors the steps below should be skipped and the packet discarded. */
#if (LORA_DEBUG) && (LORA_DEBUG_INFO)
        printf("[sx127x] [error] cannot read FIFO due to errors - packet is discarded\n");
        printf("  VALID_HEADER      %s\n", get_reg_irq_flags(lora, IRQ_VALID_HEADER, irq_flags)      ? "YES" : "NO");
        printf("  RX_DONE           %s\n", get_reg_irq_flags(lora, IRQ_RX_DONE, irq_flags)           ? "YES" : "NO");
        printf("  PAYLOAD_CRC_ERROR %s\n", get_reg_irq_flags(lora, IRQ_PAYLOAD_CRC_ERROR, irq_flags) ? "YES" : "NO");
#endif
        can_read_fifo = 0;
        lora_set_error(lora, LORA_ERROR_CANNOT_READ_FIFO);
        return;
    } 

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);  

    if (can_read_fifo)
    {  
        payload_size = get_latest_packet_received_payload_size(lora);
#if (LORA_DEBUG) && (LORA_DEBUG_RCVD_PAYLOAD_SIZE)
        printf("[sx127x] [info] received payload size %d bytes\n", payload_size);
#endif

#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_PTRS)
        get_fifo_address_pointers(lora, &fifo_addr_ptr, &fifo_tx_base_addr, &fifo_rx_base_addr, &fifo_rx_current_addr);
        printf("[sx127x] [info] ptr before read_fifo\n");
        printf("  fifo_addr_ptr           %02x\n", fifo_addr_ptr);
        printf("  fifo_tx_base_addr       %02x\n", fifo_tx_base_addr);
        printf("  fifo_rx_base_addr       %02x\n", fifo_rx_base_addr);
        printf("  fifo_rx_current_addr    %02x\n", fifo_rx_current_addr);
#endif

        // read FIFO
        if (payload_size > 0)
        {
            get_fifo_rx_current_addr(lora, &fifo_rx_current_addr);
            set_fifo_addr_ptr       (lora,  fifo_rx_current_addr);
            read_fifo(lora, io_data(lora->io_rx), payload_size);
        }
        lora->io_rx->data_size = payload_size;

#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_PTRS)
        get_fifo_address_pointers(lora, &fifo_addr_ptr, &fifo_tx_base_addr, &fifo_rx_base_addr, &fifo_rx_current_addr);
        printf("[sx127x] [info] ptr after read_fifo\n");
        printf("  fifo_addr_ptr           %02x\n", fifo_addr_ptr);
        printf("  fifo_tx_base_addr       %02x\n", fifo_tx_base_addr);
        printf("  fifo_rx_base_addr       %02x\n", fifo_rx_base_addr);
        printf("  fifo_rx_current_addr    %02x\n", fifo_rx_current_addr);
#endif
    }

    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
    {
        // Go back to SLEEP
        set_mode(lora, SLEEP);
    }
    else
    {   /* LORA_CONTINUOUS_RECEPTION */
        /* NOTE: In continuous receive mode, the modem scans the channel continuously for a preamble. */
    }
}

void sx127x_rx_async(LORA* lora, IO* io)
{
#if (LORA_DEBUG)
    uint8_t mode;
#endif
    uint8_t mode_sel;
    uint8_t fifo_tx_base_addr, fifo_rx_base_addr, fifo_addr_ptr, start_rx;
    LORA_SYS_CONFIG* sys_config = &lora->sys_config;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    
#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_COMPLETED)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;    
    }
    mode = get_mode(lora);
    if ( usr_config->recv_mode == LORA_SINGLE_RECEPTION || 
        (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && lora->rxcont_mode_started == false) )
    {
        if (mode != SLEEP)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, mode);//EXTRA_STATE_CHECK
            return;
        }
    }
    else if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && lora->rxcont_mode_started == true)
    {
        if (mode != RXCONTINUOUS)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, mode);//EXTRA_STATE_CHECK
            return;
        }  
    }
#endif    

    lora->io_rx = io;

    /* LoRaTM Receive Data FIFO Filling
        In order to write packet data into FIFO user should:
        1 Set FifoAddrPtr to FifoTxBaseAddrs.
        2 Write PayloadLength bytes to the FIFO (RegFifo) */
    fifo_tx_base_addr = sys_config->fifo_tx_base_addr; 
    fifo_rx_base_addr = sys_config->fifo_rx_base_addr;
    fifo_addr_ptr     = fifo_rx_base_addr;
    set_fifo_address_pointers(lora, fifo_addr_ptr, fifo_tx_base_addr, fifo_rx_base_addr);

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    start_rx = 0;

    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
    {
        start_rx = 1;
        mode_sel = RXSINGLE;
    }
    else if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && lora->rxcont_mode_started == false)
    {
        start_rx = 1;
        mode_sel = RXCONTINUOUS;
        lora->rxcont_mode_started = true;    
    }

    if (start_rx)
    {
#if (LORA_DEBUG)
        /* 1 Whilst in Sleep or Standby mode select RXCONT mode. */
        mode = get_mode(lora);
        if (mode != SLEEP)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, mode);//EXTRA_STATE_CHECK
            return;
        }
#endif
        /* See Figure 18. Startup Process */ 
        set_mode(lora, STDBY);
        //NOTE: no delay between changing states
        set_mode(lora, mode_sel);
    }
   
    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    lora->error = LORA_ERROR_NONE;
    //NOTE: software timer_txrx_timeout is used only for LORA_CONTINUOUS_RECEPTION
    //      (for LORA_SINGLE_RECEPTION default RX_TIMEOUT irq is used).
    if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && usr_config->rx_timeout_ms > 0)
    {
        timer_start_ms(lora->timer_txrx_timeout, usr_config->rx_timeout_ms);
    }
}

uint32_t sx127x_get_stat(LORA* lora, LORA_STAT_SEL lora_stat_sel)
{
    switch (lora_stat_sel)
    {
    case RSSI_CUR_DBM:              return /* int16_t*/get_rssi_cur_dbm(lora);
    case RSSI_LAST_PKT_RCVD_DBM:    return /* int16_t*/get_rssi_last_pkt_rcvd_dbm(lora);
    case SNR_LAST_PKT_RCVD_DB:      return /*  int8_t*/get_snr_last_pkt_rcvd_db(lora);
    case PKT_TIME_ON_AIR_MS:        return /*uint32_t*/get_pkt_time_on_air_ms(lora);
    case TX_OUTPUT_POWER_DBM:       return /*  int8_t*/get_tx_output_power_dbm(lora);
    case RF_FREQ_ERROR_HZ:          return /* int32_t*/get_rf_freq_error_hz(lora);
    default:;
    }
    return 0;
}

void sx127x_set_error(LORA* lora, LORA_ERROR error)
{
    LORA_USR_CONFIG* usr_config = &lora->usr_config;

    // clear all irqs
    set_reg_irq_flags_clear_all(lora);

    switch(error)
    {
    case LORA_ERROR_INTERNAL:
    case LORA_ERROR_CONFIG:
    case LORA_ERROR_TX_TIMEOUT:
        set_mode(lora, SLEEP);
        break;
    case LORA_ERROR_RX_TRANSFER_ABORTED:
    case LORA_ERROR_RX_TIMEOUT:
    case LORA_ERROR_PAYLOAD_CRC_INCORRECT:
    case LORA_ERROR_CANNOT_READ_FIFO:
        if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
        {
            set_mode(lora, SLEEP);
        }
        break;
    default:;
    }
}

#endif // LORA_SX127X