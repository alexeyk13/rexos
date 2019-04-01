/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#ifndef SX12XX_CONFIG_H
#define SX12XX_CONFIG_H

static bool sx12xx_config(LORA* lora)
{
    LORA_SYS_CONFIG *sys_config = &lora->sys_config;
    LORA_USR_CONFIG *usr_config = &lora->usr_config;

    /* [fifo_tx_base_addr] write base address in FIFO data buffer for TX modulator */  
    /* [fifo_rx_base_addr] read base address in FIFO data buffer for RX demodulator */  
    if (usr_config->txrx == LORA_TXRX)
    {
        /* Half of the available memory is dedicated to Rx: RegFifoRxBaseAddr initialized at address 0x00
           Half of the available memory is dedicated to Tx: RegFifoTxBaseAddr initialized at address 0x80 */
        sys_config->fifo_tx_base_addr = 0x80; 
        sys_config->fifo_rx_base_addr = 0x00;
    }
    else
    {
        sys_config->fifo_tx_base_addr = 0x00; 
        sys_config->fifo_rx_base_addr = 0x00; 
    }
    /* [rx_payload_crc_on] Enable CRC generation and check on payload:
            0 - CRC disable
            1 - CRC enable
            If CRC is needed, RxPayloadCrcOn should be set:
            - in Implicit header mode: on Tx and Rx side
            - in Explicit header mode: on the Tx side alone (recovered from
            the header in Rx side) */
    sys_config->rx_payload_crc_on = 1;
#if defined (LORA_SX127X)
    if (usr_config->chip == LORA_CHIP_SX1272)
    {
        /* [pa_select] Selects PA output pin:
                0 RFIO pin. Output power is limited to 13 dBm.
                1 PA_BOOST pin. Output power is limited to 20 dBm */
        sys_config->pa_select = usr_config->enable_high_power_pa ? 1 : 0;
        /* [tx_output_power_dbm] Power amplifier max output power:
                        Pout = 2 + OutputPower(3:0) on PA_BOOST.
                        Pout = -1 + OutputPower(3:0) on RFIO. */
        sys_config->tx_output_power_dbm = 15;
    }
    else if (usr_config->chip == LORA_CHIP_SX1276)
    {
        /* [pa_select] Selects PA output pin
                0 - RFO pin. Output power is limited to +14 dBm.
                1 - PA_BOOST pin. Output power is limited to +20 dBm */
        sys_config->pa_select = usr_config->enable_high_power_pa ? 1 : 0;
        /* [max_power] Select max output power: Pmax=10.8+0.6*MaxPower [dBm] */
        if (usr_config->enable_high_power_pa) 
            sys_config->max_power = 0x7;
        else
            sys_config->max_power = 0x4;
        /* [tx_output_power_dbm] 
                Pout=Pmax-(15-OutputPower) if PaSelect = 0 (RFO pin)
                Pout=17-(15-OutputPower) if PaSelect = 1 (PA_BOOST pin) */
        sys_config->tx_output_power_dbm = 15;  
    }
    /* [low_data_rate_optimize] LowDataRateOptimize:
            0 - Disabled
            1 - Enabled; mandated for SF11 and SF12 with BW = 125 kHz */
    sys_config->low_data_rate_optimize = 0;
    /* [detection_optimize] LoRa detection Optimize
            0x03 - SF7 to SF12
            0x05 - SF6 */
    sys_config->detection_optimize = 0x03;
    /* [detection_threshold] LoRa detection threshold
            0x0A - SF7 to SF12
            0x0C - SF6 */
    sys_config->detection_threshold = 0x0A;
    /* [fifo_addr_ptr] SPI interface address pointer in FIFO data buffer. */
    sys_config->fifo_addr_ptr = 0x00; 
    /* 
     * sx1276 related additional settings 
     */
    if (usr_config->chip == LORA_CHIP_SX1276)
    {
        /* The registers in the address space from 0x61 to 0x73 are specific for operation in
           the lower frequency bands (below 525 MHz), or in the upper frequency bands (above 779 MHz). */
        /* NOTE: For SX1272: 860 - 1020 MHz (Table 7)
           NOTE: For SX1276:     
                 Band 1 (HF) 862 - 1020 MHz (Table 32)
                 Band 2 (LF) 410 - 525  MHz (Table 32)
                 Band 3 (LF) 137 - 175  MHz (Table 32) */
        if (usr_config->rf_carrier_frequency_mhz <= 525) 
        {
            /* Sets the floor reference for all AGC thresholds:
               AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
               SNR = 8dB, fixed value */
            sys_config->agc_reference_level = 0x19;
            /* Defines the 1st AGC Threshold */
            sys_config->agc_step1 = 0x0c;
            /* Defines the 2nd AGC Threshold */
            sys_config->agc_step2 = 0x04;
            /* Defines the 3rd AGC Threshold */
            sys_config->agc_step3 = 0x0b;
            /* Defines the 4th AGC Threshold */
            sys_config->agc_step4 = 0x0c;
            /* Defines the 5th AGC Threshold */
            sys_config->agc_step5 = 0x0c;
            /* Controls the PLL bandwidth:
                00 - 75 kHz     10 - 225 kHz
                01 - 150 kHz    11 - 300 kHz */
            sys_config->pll_bandwidth = 0x03;
        } 
        else if (usr_config->rf_carrier_frequency_mhz >= 862)
        {
            /* Sets the floor reference for all AGC thresholds:
               AGC Reference[dBm]=-174dBm+10*log(2*RxBw)+SNR+AgcReferenceLevel
               SNR = 8dB, fixed value */
            sys_config->agc_reference_level = 0x1c;
            /* Defines the 1st AGC Threshold */
            sys_config->agc_step1 = 0x0e;
            /* Defines the 2nd AGC Threshold */
            sys_config->agc_step2 = 0x05;
            /* Defines the 3rd AGC Threshold */
            sys_config->agc_step3 = 0x0b;
            /* Defines the 4th AGC Threshold */
            sys_config->agc_step4 = 0x0c;
            /* Defines the 5th AGC Threshold */
            sys_config->agc_step5 = 0x0c;
            /* Controls the PLL bandwidth:
                00 - 75 kHz     10 - 225 kHz
                01 - 150 kHz    11 - 300 kHz */
            sys_config->pll_bandwidth = 0x03;
        } 
    }
    if (LORA_PREAMBLE_LEN < 8 || LORA_PREAMBLE_LEN > 65535)
    {
#if (LORA_DEBUG)
        printf("[sx12xx_config] [error] invalid preamble length\n");
#endif
        return false;
    }
    /*
     * Profiles & depended settings
     */   
    if (usr_config->enable_SF_6)
    {
        /* Spreading Factor 6
           SF = 6 Is a special use case for the highest data rate transmission possible
           with the LoRa modem. To this end several settings must be activated in the
           SX1272/73 registers when it is in use. These settings are only valid for SF6
           and should be set back to their default values for other spreading factors:
           - Set SpreadingFactor = 6 in RegModemConfig2
           - The header must be set to Implicit mode.
           - Set the bit field DetectionOptimize of register RegLoRaDetectOptimize to
             value "0b101".
           - Write 0x0C in the register RegDetectionThreshold. */
        usr_config->spreading_factor        = 6;
        usr_config->implicit_header_mode_on = LORA_IMPLICIT_HEADER_MODE;
        sys_config->detection_optimize      = 0x05;
        sys_config->detection_threshold     = 0x0C;
#if (LORA_DEBUG)
        printf("[sx12xx_config] [info] enabled spreading factor 6\n");
#endif
    }
    /* [signal_bandwidth] Signal bandwidth:
            00 - 125 kHz
            01 - 250 kHz
            10 - 500 kHz
            11 - reserved */ 
    if ((usr_config->spreading_factor == 11 || usr_config->spreading_factor == 12) && usr_config->signal_bandwidth == 0)
    {
        /* [low_data_rate_optimize] LowDataRateOptimize:
        0 - Disabled
        1 - Enabled; mandated for SF11 and SF12 with BW = 125 kHz */
        sys_config->low_data_rate_optimize = 1;
#if (LORA_DEBUG)
        printf("[sx12xx_config] [info] enabled low data rate optimization (mandated case)\n");
#endif
    }
#elif defined (LORA_SX126X)
    /* 9.2.1 Image Calibration for Specific Frequency Bands
       The image calibration is done through the command CalibrateImage(...) 
       for a given range of frequencies defined by the parameters freq1 and 
       freq2. Once performed, the calibration is valid for all frequencies 
       between the two extremes used as parameters. Typically, the user can 
       select the parameters freq1 and freq2 to cover any specific ISM band. */
    /* Frequency Band [MHz]    Freq1           Freq2
       430 - 440               0x6B            0x6F
       470 - 510               0x75            0x81
       779 - 787               0xC1            0xC5
       863 - 870               0xD7            0xDB
       902 - 928               0xE1 (default)  0xE9 (default) */
    if (usr_config->rf_carrier_frequency_mhz >= 430 && usr_config->rf_carrier_frequency_mhz <= 440)
    {
        sys_config->freq1 = 0x6B;   sys_config->freq2 = 0x6F;           
    }
    else if (usr_config->rf_carrier_frequency_mhz >= 470 && usr_config->rf_carrier_frequency_mhz <= 510)
    {
        sys_config->freq1 = 0x75;   sys_config->freq2 = 0x81;           
    }   
    else if (usr_config->rf_carrier_frequency_mhz >= 779 && usr_config->rf_carrier_frequency_mhz <= 787)
    {
        sys_config->freq1 = 0xC1;   sys_config->freq2 = 0xC5;           
    }
    else if (usr_config->rf_carrier_frequency_mhz >= 863 && usr_config->rf_carrier_frequency_mhz <= 870)
    {
        sys_config->freq1 = 0xD7;   sys_config->freq2 = 0xDB;           
    } 
    else if (usr_config->rf_carrier_frequency_mhz >= 902 && usr_config->rf_carrier_frequency_mhz <= 928)
    {
        sys_config->freq1 = 0xE1;   sys_config->freq2 = 0xE9;           
    } 
    /* The TX output power is defined as power in dBm in a range of
       -17 (0xEF) to +14 (0x0E) dBm by step of 1 dB if low  power PA is selected
       -9  (0xF7) to +22 (0x16) dBm by step of 1 dB if high power PA is selected */
    //NOTE: The default maximum RF output power of the transmitter is
    //      +14/15 dBm for SX1261 and +22 dBm for SX1262.
    if (usr_config->enable_high_power_pa) 
        sys_config->tx_output_power_dbm = 15;
    else
        sys_config->tx_output_power_dbm = 14; // By default low power PA and +14 dBm are set.

    if (!((usr_config->enable_high_power_pa == 0 && (sys_config->tx_output_power_dbm >= -17 &&
                                                     sys_config->tx_output_power_dbm <=  14)) ||
          (usr_config->enable_high_power_pa == 1 && (sys_config->tx_output_power_dbm >=  -9 &&
                                                     sys_config->tx_output_power_dbm <=  15))))
    {
#if (LORA_DEBUG)
        printf("[sx12xx_config] [error] incorrect tx_output_power_dbm\n");
#endif
        return false; 
    }
    /* The transmitted preamble length may vary from 10 to 65535 symbols */
    if (LORA_PREAMBLE_LEN < 10 || LORA_PREAMBLE_LEN > 65535)
    {
#if (LORA_DEBUG)
        printf("[sx12xx_config] [error] invalid preamble length\n");
#endif
        return false;
    }
    /* CRC Information extracted from the received packet header
        (Explicit header mode only)
        0 - Header indicates CRC off
        1 - Header indicates CRC on */
    //NOTE: for sx126x set always to 1 (see get_crc_info_from_rcvd_pkt_header for details)
    sys_config->rx_payload_crc_on = 1;
    /* [low_data_rate_optimize] LowDataRateOptimize:
            0 - Disabled
            1 - Enabled; mandated for * 
       * SX1261:
       - LDO is mandated for SF11 and SF12 with BW = 125 kHz
       - LDO is mandated for SF12          with BW = 250 kHz */
    sys_config->low_data_rate_optimize = 0;
    /* Signal bandwidth (SX1261):
       0x00 - 7.81  kHz       0x08 - 10.42 kHz
       0x01 - 15.63 kHz       0x09 - 20.83 kHz
       0x02 - 31.25 kHz       0x0A - 41.67 kHz
       0x03 - 62.50 kHz       0x04 - 125   kHz
       0x05 - 250   kHz       0x06 - 500   kHz */
    if ((usr_config->spreading_factor == 11 && usr_config->signal_bandwidth == 0x04) ||
        (usr_config->spreading_factor == 12 && usr_config->signal_bandwidth == 0x04) ||
        (usr_config->spreading_factor == 12 && usr_config->signal_bandwidth == 0x05))
    {
        sys_config->low_data_rate_optimize = 1;
#if (LORA_DEBUG)
        printf("[sx12xx_config] [info] enabled low data rate optimization (mandated case)\n");
#endif
    }
#endif
    return true;
}

#endif //SX12XX_CONFIG_H