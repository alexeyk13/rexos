/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#include "../userspace/gpio.h"
#include "loras_private.h"

#if defined (LORA_SX126X) 

#include "sx126x.h"

//#define SPI_TEST

#if defined(SPI_TEST)
void sx126x_spi_test(LORA* lora) 
{
    const int test_sel = 2;
    uint8_t reg_data = 0, cnt = 0;

    if (test_sel == 0)
    {
        chip_mode_t chip_mode;
        while (1)
        {
            chip_mode = get_chip_mode (lora);
            printf("[sx126x] [info] chip_mode %d\n", chip_mode);
            sleep_ms(1000);
        } 
    }
    else if (test_sel == 1) //sleep test
    {
        while (1)    
        {
            printf("[sx126x] [info] before CS_SLEEP_WARM_START\n");
            set_state(lora, CS_SLEEP_WARM_START);
            printf("[sx126x] [info] after  CS_SLEEP_WARM_START\n\n"); 
            sleep_ms(1000);
            printf("[sx126x] [info] before CS_STDBY_RC\n");            
            set_state(lora, CS_STDBY_RC);
            printf("[sx126x] [info] after  CS_STDBY_RC\n\n");
            sleep_ms(1000); 
        } 
    }
    else if (test_sel == 2) 
    {
        while (1) 
        {
            read_reg(lora, REG_LORA_SYNC_WORD_MSB, &reg_data);
            printf("[sx126x] [info] reg_data %x\n", reg_data);//0x14 expected
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

void sx126x_open(LORA* lora)  
{    
#if (LORA_DEBUG)
    chip_state_t chip_state;
#endif
    LORA_SYS_CONFIG* sys_config = &lora->sys_config;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;

    //POR & manual reset
    if (LORA_DO_MANUAL_RESET_AT_OPEN) 
    {
        set_state(lora, CS_RESET);
#if (LORA_DEBUG)
        printf("[sx126x] [info] manual reset completed\n");
#endif
        /* 1. Calibration procedure is automatically called in case of POR
           2. Once the calibration is finished, the chip enters STDBY_RC mode 
           3. By default, after battery insertion or reset operation (pin NRESET
              goes low), the chip will enter in STDBY_RC mode running with a 13
              MHz RC clock. */
        /* Once the calibration is finished, the chip enters STDBY_RC mode. */
#if (LORA_DEBUG)
        chip_state = get_state_cur(lora);
        if (chip_state != LORA_STDBY_TYPE)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, chip_state);//EXTRA_STATE_CHECK
            return;
        }
#endif
    }
    else  
    {
        /* 1.   If not in STDBY_RC mode, then go to this mode with the command SetStandby(...) */
        /* 13.1.2 SetStandby
           The command SetStandby(...) is used to set the device in a 
           configuration mode which is at an intermediate level of consumption. 
           In this mode, the chip is placed in halt mode waiting for instructions
           via SPI. This mode is dedicated to chip configuration using high 
           level commands such as SetPacketType(...). */
        set_state(lora, LORA_STDBY_TYPE); 
    }  

#if defined(SPI_TEST)
    sx126x_spi_test(lora);
#endif  

    /* DIO2 can be configured to drive an RF switch through the use 
       of the command SetDio2AsRfSwitchCtrl(...). In this mode, DIO2
       will be at a logical 1 during Tx and at a logical 0 in any 
       other mode */
    if (usr_config->dio2_as_rf_switch_ctrl)
    {
        set_dio2_as_rf_switch_ctrl(lora, true);
    }

    /* DIO3 can be used to automatically control a TCXO through the command 
       SetDio3AsTCXOCtrl(...). In this case, the device will automatically power 
       cycle the TCXO when needed. */
    if (usr_config->dio3_as_tcxo_ctrl)
    {
        if (!set_dio3_as_tcxo_ctrl(lora))
        {
#if (LORA_DEBUG)
            lora_set_intern_error(lora, __FILE__, __LINE__, 0);
#endif
            error(ERROR_NOT_CONFIGURED);
            return;
        }
    }

    /* 1. Calibration procedure is automatically called in case of POR
       2. Once the calibration is finished, the chip enters STDBY_RC mode 
       3. By default, after battery insertion or reset operation (pin NRESET
          goes low), the chip will enter in STDBY_RC mode running with a 13
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
       Once the calibration is finished, the chip enters STDBY_RC mode. */
    if (LORA_DO_MANUAL_CALIBRATION)
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
        calibrate (lora, calib_param);
 
        /* 9.2.1 Image Calibration for Specific Frequency Bands
           The image calibration is done through the command CalibrateImage(...) 
           for a given range of frequencies defined by the parameters freq1 and 
           freq2. Once performed, the calibration is valid for all frequencies 
           between the two extremes used as parameters. Typically, the user can 
           select the parameters freq1 and freq2 to cover any specific ISM band. */
        calibrate_image (lora, sys_config->freq1, sys_config->freq2);
    }
    

    //configure modem

    /* 2.   Define the protocol (LoRa® or FSK) with the command SetPacketType(...) */
    /* The user specifies the modem and frame type by using the command 
       SetPacketType(...). This command specifies the frame used and 
       consequently the modem implemented.
       This function is the first one to be called before going to Rx or Tx 
       and before defining modulation and packet parameters. The command 
       GetPacketType() returns the current protocol of the radio. */
    set_packet_type (lora, PACKET_TYPE_LORA);

    /* 3.   Define the RF frequency with the command SetRfFrequency(...) */
    set_rf_frequency_mhz (lora, usr_config->rf_carrier_frequency_mhz);

    /* 4.   Define output power and ramping time with the command SetTxParams(...) */
    if (lora->tx_on)
    {
        set_tx_params(lora, sys_config->tx_output_power_dbm);
    }

    /* 5.   Define where the data payload will be stored with the command SetBufferBaseAddress(...) */
    set_buffer_base_address (lora, sys_config->fifo_tx_base_addr, sys_config->fifo_rx_base_addr);

    /* 6.   Send the payload to the data buffer with the command WriteBuffer(...) */
    //NOTE: will be done in sx126x_tx_async

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
    set_modulation_params (lora, usr_config->spreading_factor,
        usr_config->signal_bandwidth, LORA_CODING_RATE, sys_config->low_data_rate_optimize);


    /* 8.   Define the frame format to be used with the command SetPacketParams(...) */
    //NOTE: need pl for time_on_air_ms calc. (to check if timer ms values are reasonable)
    set_packet_params_ext(lora, usr_config->payload_length);

    /* 9.   Configure DIO and IRQ: use the command SetDioIrqParams(...) to select TxDone IRQ and
            map this IRQ to a DIO (DIO1, DIO2 or DIO3) */
    /* By default, all IRQ are masked (all ‘0’) and the user 
       can enable them one by one (or several at a time) by 
       setting the corresponding mask to ‘1’.*/
    uint16_t irq_mask = 0x3FF;//enable all irqs
    set_dio_irq_params (lora, irq_mask, usr_config->dio1_mask, usr_config->dio2_mask, usr_config->dio3_mask);  
 
    /* 10.  Define Sync Word value: use the command WriteReg(...) to write the value of the register
            via direct register access */ 
    write_reg (lora, REG_LORA_SYNC_WORD_MSB, (LORA_SYNC_WORD_SX126X >> 8) & 0xff);
    write_reg (lora, REG_LORA_SYNC_WORD_LSB, (LORA_SYNC_WORD_SX126X >> 0) & 0xff);

	//Select LDO or DC_DC+LDO for CFG_XOSC, FS, RX or TX mode
    set_regulator_mode(lora);

    //enter sleep mode
    set_state(lora, CS_SLEEP_WARM_START); 
}

void sx126x_close(LORA* lora) 
{
    if (get_state_cur(lora) != LORA_STDBY_TYPE)
    {
        set_state(lora, LORA_STDBY_TYPE);
    }
    set_state(lora, CS_SLEEP_COLD_START); 
}

void sx126x_tx_async_wait(LORA* lora)
{
    uint16_t irq_status;  
#if (LORA_DEBUG)
    chip_state_t chip_state;   
#endif    

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;
    }
    //NOTE: Do not know if packet is already sent (state TX or STDBY)
    //      => do not check state
#endif

    /* 12.  Wait for the IRQ TxDone or Timeout: once the packet has been sent the chip goes
            automatically to STDBY_RC mode */
    get_irq_status(lora, &irq_status);
    if (is_irq_set(lora, IRQ_TX_DONE, irq_status))
    {
#if (LORA_DEBUG) && (LORA_DEBUG_IRQS)
        printf("[sx126x] [info] got irq 0x%04x (TX_DONE)\n", irq_status);
#endif
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else if (is_irq_set(lora, IRQ_TIMEOUT, irq_status))
    {
#if (LORA_DEBUG) && (LORA_DEBUG_IRQS)
        printf("[sx126x] [info] got irq 0x%04x (TIMEOUT)\n", irq_status);
#endif
        lora_set_error(lora, LORA_ERROR_TX_TIMEOUT);
        return;
    }
    else
    {
        return;//no irq
    }

#if (LORA_DEBUG)
    // chip auto-goes back to STDBY
    chip_state = get_state_cur(lora);
    if (chip_state != LORA_STDBY_TYPE)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, chip_state);//EXTRA_STATE_CHECK
        return;  
    }
#endif

    /* 13.  Clear the IRQ TxDone flag */
    //clear_irq_status_irq (lora, TX_DONE);
    clear_irq_status_all(lora);

#if (LORA_DEBUG) && (LORA_DEBUG_FIFO_DATA)
    //debug FIFO if needed
#endif

    // Go back from STDBY_RC to SLEEP
    set_state(lora, CS_SLEEP_WARM_START);
}

void sx126x_tx_async(LORA* lora, IO* io)
{
#if (LORA_DEBUG)
    chip_state_t chip_state;
#endif    
    LORA_SYS_CONFIG* sys_config = &lora->sys_config;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;

    /* 1.   If not in STDBY_RC mode, then go to this mode with the command SetStandby(...) */
    /* 13.1.2 SetStandby
       The command SetStandby(...) is used to set the device in a 
       configuration mode which is at an intermediate level of consumption. 
       In this mode, the chip is placed in halt mode waiting for instructions
       via SPI. This mode is dedicated to chip configuration using high 
       level commands such as SetPacketType(...). */
#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_COMPLETED)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;    
    }
    chip_state = get_state_cur(lora);
    if (chip_state != CS_SLEEP_COLD_START && chip_state != CS_SLEEP_WARM_START)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, chip_state);//EXTRA_STATE_CHECK
        return;
    }
#endif
    set_state(lora, LORA_STDBY_TYPE); 

    /* 6.   Send the payload to the data buffer with the command WriteBuffer(...) */
    /* The parameter offset defines the address pointer of the first data to be
       written or read. Offset zero defines the first position of the data buffer. */
    write_buffer (lora, sys_config->fifo_tx_base_addr, io_data(io), io->data_size);

    // Set payload length as size of current data
    set_payload_length(lora, io->data_size);

    // clear all irqs
    clear_irq_status_all(lora); 
       
    /* 11.  Set the circuit in transmitter mode to start transmission with the command SetTx().  
            Use the parameter to enable Timeout */
    set_state(lora, CS_TX, usr_config->tx_timeout_ms);
       
    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    lora->error = LORA_ERROR_NONE;
}

void sx126x_rx_async_wait(LORA* lora)
{
#if (LORA_DEBUG)
    chip_state_t chip_state;
#endif 
    uint16_t irq_status;
    uint8_t can_read_fifo, payload_length_rx, rx_start_buffer_pointer;
    bool condition;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;    
    }
    chip_state = get_state_cur(lora);
    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
    {
        //Single mode: the device returns automatically to STDBY_RC mode after packet reception
        //NOTE: Do not know if packet is received (state is RX or STDBY)
        //      => do not check state
    }
    else if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION)
    {
        if (chip_state != CS_RX)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, chip_state);//EXTRA_STATE_CHECK
            return;
        }  
    }
#endif

    /* 10.  Wait for IRQ RxDone or Timeout: the chip will stay in Rx and look 
       for a new packet if the continuous mode is selected otherwise it will 
       goes to STDBY_RC mode. */
    get_irq_status(lora, &irq_status);
    // wait for RX_DONE IRQ
    if (is_irq_set(lora, IRQ_RX_DONE, irq_status))
    {
#if (LORA_DEBUG) && (LORA_DEBUG_IRQS)
        printf("[sx126x] [info] got irq 0x%04x (RX_DONE)\n", irq_status);
#endif
        lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    }
    else if (usr_config->recv_mode == LORA_SINGLE_RECEPTION && is_irq_set(lora, IRQ_TIMEOUT, irq_status))
    {
        lora_set_error(lora, LORA_ERROR_RX_TIMEOUT);
        return;
    }
    else
    {
        return;//no irq
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
    if (usr_config->implicit_header_mode_on == 1 || get_crc_info_from_rcvd_pkt_header(lora) == 1)
    {
        if (is_irq_set(lora, IRQ_CRC_ERR, irq_status))
        {
#if (LORA_DEBUG) && (LORA_DEBUG_IRQS)
            printf("[sx126x] [info] got irq 0x%04x (CRC_ERR) wait_irq_cnt %d\n", irq_status, wait_irq_cnt);
#endif
            lora_set_error(lora, LORA_ERROR_PAYLOAD_CRC_INCORRECT);
            return;
        }
    }

    can_read_fifo = 1;

    condition = is_irq_set(lora, IRQ_RX_DONE, irq_status) && 
                is_irq_set(lora, IRQ_CRC_ERR, irq_status) == 0;

    if (!condition)
    {
        /* In case of errors the steps below should be skipped and the packet discarded. */
#if (LORA_DEBUG) && (LORA_DEBUG_INFO)
        printf("[sx126x] cannot read FIFO due to errors - packet is discarded\n");
        printf("[sx126x] RX_DONE  %s\n", is_irq_set(lora, IRQ_RX_DONE, irq_status) ? "YES" : "NO");
        printf("[sx126x] CRC_ERR  %s\n", is_irq_set(lora, IRQ_CRC_ERR, irq_status) ? "YES" : "NO");
#endif
        can_read_fifo = 0;
        lora_set_error(lora, LORA_ERROR_CANNOT_READ_FIFO);
        return;
    } 

    // clear all irqs
    clear_irq_status_all(lora); 

    /* In case of a valid packet (CRC Ok), get the packet length and
       address of the first byte of the received payload by using 
       the command GetRxBufferStatus(...) */
    /* 13.  In case of a valid packet (CRC Ok), start reading the packet */
    if (can_read_fifo)
    {  
        get_rx_buffer_status(lora, &payload_length_rx, &rx_start_buffer_pointer);
#if (LORA_DEBUG) && (LORA_DEBUG_RCVD_PAYLOAD_SIZE)
        printf("[sx126x] received payload size %d bytes\n", payload_length_rx);
#endif
        if (payload_length_rx > 0)
        {
            read_buffer(lora, rx_start_buffer_pointer, io_data(lora->io_rx), payload_length_rx);
        }
        lora->io_rx->data_size = payload_length_rx;
    }

    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
    {
        // Go back to SLEEP
        set_state(lora, CS_SLEEP_WARM_START); 
    }
    else
    {   /* LORA_CONTINUOUS_RECEPTION */
        /* 0xFFFFFF Rx Continuous mode. The device remains in RX mode until 
           the host sends a command to change the operation mode. The device can 
           receive several packets. Each time a packet is received, a packet done
           indication is given to the host and the device will automatically 
           search for a new packet. */
    }
}

void sx126x_rx_async(LORA* lora, IO* io)
{
    uint8_t start_rx;
    uint32_t timeout;
#if (LORA_DEBUG)
    chip_state_t chip_state;
#endif
    LORA_USR_CONFIG* usr_config = &lora->usr_config;

#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_COMPLETED)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;    
    }
    chip_state = get_state_cur(lora);
    if ( usr_config->recv_mode == LORA_SINGLE_RECEPTION || 
        (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && lora->rxcont_mode_started == false) )
    {
        if (chip_state != CS_SLEEP_COLD_START && chip_state != CS_SLEEP_WARM_START)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, chip_state);//EXTRA_STATE_CHECK
            return;
        }
    }
    else if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && lora->rxcont_mode_started == true)
    {
        if (chip_state != CS_RX)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, chip_state);//EXTRA_STATE_CHECK
            return;
        }  
    }
#endif

    lora->io_rx = io;
    start_rx = 0;

    /* NOTE1: "No timeout" (0x000000) looks useless cause it locks => not support it for now
       NOTE2: "SINGLE_RECEPTION"     in sx126x != "Rx Single mode"     in sx126x
                 ("SINGLE_RECEPTION" in sx126x == "Timeout active" in sx126x)
       NOTE3: "CONTINUOUS_RECEPTION" in sx126x == "Rx Continuous mode" in sx126x */
    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
    {
        set_state(lora, LORA_STDBY_TYPE); 
        start_rx = 1;
        /* By default, the timer will be stopped only if the Sync Word or header 
           has been detected. However, it is also possible to stop the timer upon
           preamble detection by using the command StopTimerOnPreamble(...). */
        //NOTE: this is to make behavior for the end user similar between sx126x and sx126x
        //      => stop the timer upon preamble detection
        if (usr_config->rx_timeout_ms > 0 && usr_config->rx_timeout_ms < SX126X_TIMEOUT_MAX_VAL)
        {
            stop_timer_on_preamble(lora, STOP_ON_PREAMBLE_ENABLE);
        }
    }
    else if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && lora->rxcont_mode_started == false)
    {
        set_state(lora, LORA_STDBY_TYPE); 
        start_rx = 1;
        lora->rxcont_mode_started = true;    
    }

    // clear all irqs
    clear_irq_status_all(lora);		
		
    if (start_rx)
    {
#if (LORA_DEBUG)
        chip_state = get_state_cur(lora);
        if (chip_state != LORA_STDBY_TYPE)
        {
            lora_set_intern_error(lora, __FILE__, __LINE__, 1, chip_state);//EXTRA_STATE_CHECK
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
        if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
        {
            timeout = usr_config->rx_timeout_ms;
        }
        else if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION)
        {
            timeout = 0xFFFFFF;
        }
        set_state(lora, CS_RX, timeout);
    }
    
    lora->status = LORA_STATUS_TRANSFER_IN_PROGRESS;
    lora->error = LORA_ERROR_NONE;
    //NOTE: software timer_txrx_timeout is used only for LORA_CONTINUOUS_RECEPTION
    //      (for LORA_SINGLE_RECEPTION internal rx_timeout_ms is used).
    if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION && usr_config->rx_timeout_ms > 0)
    {
        timer_start_ms(lora->timer_txrx_timeout, usr_config->rx_timeout_ms);
    }
}
static inline uint32_t sx126x_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}
static inline uint32_t sx126x_ceil(uint32_t exp, uint32_t fra)
{ 
    return (fra > 0) ? exp+1 : exp;
}
//NOTE: based on sx126x time-on-air calculation + adapted for sx126x
static inline uint32_t get_pkt_time_on_air_ms(LORA* lora)
{
    uint32_t bw, sf, r_s, t_s, pl, ih, de, crc, cr, exp, fra, dividend, divisor;
    uint32_t n_preamble, n_payload, t_preamble, t_payload, t_packet;
    const uint32_t domain_us = 1000000;//to keep precision with integer arith.

    bw = get_bandwidth_hz(lora);
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

    //see 6.1.4 LoRa Time-on-Air
    if (sf == 5 || sf == 6)
    {
        dividend = sx126x_max((8*pl) + (16*crc) - (4*sf) + (20*ih), 0);
        divisor  = (4*sf);
    }
    else if (de == 0)
    {
        dividend = sx126x_max((8*pl) + (16*crc) - (4*sf) + 8 + (20*ih), 0);
        divisor  = (4*sf);
    }
    else
    {
        dividend = sx126x_max((8*pl) + (16*crc) - (4*sf) + 8 + (20*ih), 0);
        divisor  = (4*(sf-2));
    }

    exp = dividend / divisor;
    fra = ((dividend * 10) / divisor) - (exp * 10);//get 1st digit of fractional part

    // number of payload symbols
    if (sf == 5 || sf == 6)
    {
        n_payload = 6.25 + 8 + sx126x_ceil(exp, fra) * (cr+4);
    }
    else
    {
        n_payload = 4.25 + 8 + sx126x_ceil(exp, fra) * (cr+4);
    }
    // payload duration
    t_payload = n_payload * t_s;
    // total packet time on air
    t_packet = t_preamble + t_payload;
    return t_packet / 1000;
}
static inline int8_t get_tx_output_power_dbm(LORA* lora)
{
    //TX output power
    return lora->sys_config.tx_output_power_dbm;
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
    read_reg (lora, REG_FREQ_ERROR+0, &regval); f_err |= (regval << 16);
    read_reg (lora, REG_FREQ_ERROR+1, &regval); f_err |= (regval << 8);
    read_reg (lora, REG_FREQ_ERROR+2, &regval); f_err |= (regval << 0);
    //NOTE: REG_FREQ_ERROR (0x076B) seems not documented => cast it to int32_t...
    f_err_i = (int32_t)f_err;
    return f_err_i; 
}

uint32_t sx126x_get_stat(LORA* lora, LORA_STAT_SEL lora_stat_sel)
{
    chip_state_t chip_state;
    uint32_t stat;
    bool was_sleep = false;

    chip_state = get_state_cur(lora);
    if (chip_state == CS_SLEEP_COLD_START || chip_state == CS_SLEEP_WARM_START)
    {
        was_sleep = true;
        /* In Sleep mode, the BUSY pin is held high through a 20 kΩ resistor
           and the BUSY line will go low as soon as the radio leaves the Sleep
           mode. */
        set_state(lora, LORA_STDBY_TYPE);//wake-up chip (will set BUSY to low)            
    }

    switch (lora_stat_sel)
    {
    case RSSI_CUR_DBM:           stat = /* int16_t*/get_rssi_cur_dbm(lora);           break;  
    case RSSI_LAST_PKT_RCVD_DBM: stat = /* int16_t*/get_rssi_last_pkt_rcvd_dbm(lora); break;  
    case SNR_LAST_PKT_RCVD_DB:   stat = /*  int8_t*/get_snr_last_pkt_rcvd_db(lora);   break;  
    case PKT_TIME_ON_AIR_MS:     stat = /*uint32_t*/get_pkt_time_on_air_ms(lora);     break;  
    case TX_OUTPUT_POWER_DBM:    stat = /*  int8_t*/get_tx_output_power_dbm(lora);    break;  
    case RF_FREQ_ERROR_HZ:       stat = /* int32_t*/get_rf_freq_error_hz(lora);       break;  
    default:                     stat = 0;
    }
 
    if (was_sleep) 
    {
        /* In Sleep mode, the BUSY pin is held high through a 20 kΩ resistor
           and the BUSY line will go low as soon as the radio leaves the Sleep
           mode. */
        set_state(lora, CS_SLEEP_WARM_START);
    }

    return stat;
}

void sx126x_set_error(LORA* lora, LORA_ERROR error)
{
    LORA_USR_CONFIG* usr_config = &lora->usr_config;

    // clear all irqs
    clear_irq_status_all(lora);

    switch(error)
    {
    case LORA_ERROR_INTERNAL:
    case LORA_ERROR_CONFIG:
    case LORA_ERROR_TX_TIMEOUT:
        set_state(lora, CS_SLEEP_WARM_START);
        break;
	case LORA_ERROR_RX_TRANSFER_ABORTED:
    case LORA_ERROR_RX_TIMEOUT:
    case LORA_ERROR_PAYLOAD_CRC_INCORRECT:
    case LORA_ERROR_CANNOT_READ_FIFO:
        if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
        {
            set_state(lora, CS_SLEEP_WARM_START);
        }
        break;
    default:;
    }
}

#endif // LORA_SX126X