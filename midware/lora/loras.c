/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#include "sys_config.h"
#include "config.h"

#include "process.h"
#include "stdio.h"
#include "sys.h"
#include "io.h"
#include "spi.h"
#include "loras_private.h"
#include "../userspace/gpio.h"
#include "pin.h"
#include "power.h"

#include <string.h>

#if defined (LORA_SX127X) || defined (LORA_SX126X)

#include "sx12xx_config.h"

/* SPI command formats: 
   SX127X: address (1 byte) + data 
   SX126X: command (1 byte) + extra_data (N bytes) + data
   NOTE  : extra_data can be: reg address (2 bytes), NOP (1 byte), offset (1 byte), etc */
#define LORA_SPI_EXTRA_BYTES_MAX      (20) //should be enough
#define LORA_SPI_IO_SIZE              (LORA_SPI_EXTRA_BYTES_MAX+LORA_MAX_PAYLOAD_LEN)

#if defined (LORA_SX127X)
void sx127x_open (LORA* lora);
void sx127x_close(LORA* lora);
void sx127x_tx_async (LORA* lora, IO* io);
void sx127x_rx_async (LORA* lora, IO* io); 
void sx127x_tx_async_wait (LORA* lora);
void sx127x_rx_async_wait (LORA* lora);
void sx127x_set_error(LORA* lora, LORA_ERROR error);
uint32_t sx127x_get_stat(LORA* lora, LORA_STAT_SEL lora_stat_sel);
#elif defined (LORA_SX126X)
void sx126x_open (LORA* lora);
void sx126x_close(LORA* lora);
void sx126x_tx_async (LORA* lora, IO* io);
void sx126x_rx_async (LORA* lora, IO* io); 
void sx126x_tx_async_wait (LORA* lora);
void sx126x_rx_async_wait (LORA* lora);
void sx126x_set_error(LORA* lora, LORA_ERROR error);
uint32_t sx126x_get_stat(LORA* lora, LORA_STAT_SEL lora_stat_sel);
#endif

uint32_t lora_get_stat(LORA* lora, LORA_STAT_SEL lora_stat_sel)
{
#if defined (LORA_SX127X)
    return sx127x_get_stat(lora, lora_stat_sel);
#elif defined (LORA_SX126X)
    return sx126x_get_stat(lora, lora_stat_sel);
#endif
    return 0;
}

#if (LORA_DEBUG)
uint32_t lora_get_uptime_ms()
{
    SYSTIME uptime;
    uint32_t ms_passed;
    get_uptime(&uptime);
    ms_passed = (uptime.sec * 1000) + (uptime.usec / 1000);
    return ms_passed;     
}
#endif

void lora_set_error(LORA* lora, LORA_ERROR error)
{
    switch(error)
    {
    case LORA_ERROR_NONE:                                                       break;
    case LORA_ERROR_RX_TRANSFER_ABORTED:  /*++lora->stats.xxx;*/                break;
    case LORA_ERROR_TX_TIMEOUT:             ++lora->stats.tx_timeout_num;       break;
    case LORA_ERROR_RX_TIMEOUT:             ++lora->stats.rx_timeout_num;       break;
    case LORA_ERROR_PAYLOAD_CRC_INCORRECT:  ++lora->stats.crc_err_num;          break;
    case LORA_ERROR_CANNOT_READ_FIFO:     /*++lora->stats.xxx;*/                break;
    case LORA_ERROR_INTERNAL:             /*++lora->stats.xxx;*/                break;
    case LORA_ERROR_CONFIG:               /*++lora->stats.xxx;*/                break;
#if defined (LORA_SX127X)
    case LORA_ERROR_NO_CRC_ON_THE_PAYLOAD:/*++lora->stats.xxx;*/                break;
#endif
    default:;
    }
    lora->error  = error;
    lora->status = LORA_STATUS_TRANSFER_COMPLETED;
#if defined (LORA_SX127X)
    sx127x_set_error(lora, error);
#elif defined (LORA_SX126X)
    sx126x_set_error(lora, error);
#endif
}

#if (LORA_DEBUG)
void lora_set_intern_error(LORA* lora, const char* file, const uint32_t line, uint32_t argc, ...)
{
    va_list va;
    uint32_t arg, i;
    printf("[loras] [internal error] %s:%u now_ms:%u", file, line, lora_get_uptime_ms());
    va_start(va, argc);
    for (i = 0; i < argc; ++i)
    {
        arg = va_arg(va, uint32_t);
        printf(" 0x%x", arg);
    }
    printf("\n");
    va_end(va);
    lora_set_error(lora, LORA_ERROR_INTERNAL);
    while(1);
} 
void lora_set_config_error(LORA* lora, const char* file, const uint32_t line, uint32_t argc, ...)
{
    va_list va;
    uint32_t arg, i;
    printf("[loras] [config error] %s:%u now_ms:%u", file, line, lora_get_uptime_ms());
    va_start(va, argc);
    for (i = 0; i < argc; ++i)
    {
        arg = va_arg(va, uint32_t);
        printf(" 0x%x", arg);
    }
    printf("\n");
    va_end(va);
    lora_set_error(lora, LORA_ERROR_CONFIG);
    while(1);
}
#endif

static void lora_timer_txrx_timeout(LORA* lora)
{
#if (LORA_DEBUG)
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;
    }
#endif

    //stop calling lora_timer_poll_timeout
    timer_stop(lora->timer_poll_timeout, 0, HAL_LORA);
    lora->timer_poll_timeout_stopped = true;

    switch (lora->req)
    {
    case LORA_REQ_SEND: lora_set_error(lora, LORA_ERROR_TX_TIMEOUT); break;
    case LORA_REQ_RECV: lora_set_error(lora, LORA_ERROR_RX_TIMEOUT); break;
    default:
#if (LORA_DEBUG)
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->req);
#endif
        error(ERROR_NOT_SUPPORTED);
        return;
    }
#if (LORA_DEBUG)
    printf("[loras] [info] timer_txrx_timeout expired -> transfer completed\n");
#endif
    ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_TRANSFER_COMPLETED), lora->error, 0, 0);
}

static void lora_clear_stats_s(LORA* lora)
{
    memset(&lora->stats, 0, sizeof(LORA_STATS));
}

static bool lora_usr_config_valid(LORA* lora)
{
    LORA_USR_CONFIG *usr_config = &lora->usr_config;

    if (!(usr_config->txrx == LORA_TX || usr_config->txrx == LORA_RX))
        return false;

#if defined (LORA_SX127X) || defined (LORA_SX126X)
    if (!(usr_config->txrx == LORA_TX || usr_config->txrx == LORA_RX))
        return false;
    if (!(usr_config->recv_mode == LORA_SINGLE_RECEPTION || 
          usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION))
        return false;
    if (!(usr_config->implicit_header_mode_on == 0 || usr_config->implicit_header_mode_on == 1))
        return false;
#if defined (LORA_SX127X)
    if (usr_config->chip == LORA_CHIP_SX1272 &&
       !(usr_config->rf_carrier_frequency_mhz >= 860 && usr_config->rf_carrier_frequency_mhz <= 1020))
        return false;
    else if (usr_config->chip == LORA_CHIP_SX1276 &&
            !(usr_config->rf_carrier_frequency_mhz >= 137 && usr_config->rf_carrier_frequency_mhz <= 175) &&
            !(usr_config->rf_carrier_frequency_mhz >= 410 && usr_config->rf_carrier_frequency_mhz <= 525) &&
            !(usr_config->rf_carrier_frequency_mhz >= 862 && usr_config->rf_carrier_frequency_mhz <= 1020))
        return false;
#elif defined (LORA_SX126X)
    if (!(usr_config->rf_carrier_frequency_mhz >= 430 && usr_config->rf_carrier_frequency_mhz <= 440) &&
        !(usr_config->rf_carrier_frequency_mhz >= 470 && usr_config->rf_carrier_frequency_mhz <= 510) &&
        !(usr_config->rf_carrier_frequency_mhz >= 779 && usr_config->rf_carrier_frequency_mhz <= 787) &&
        !(usr_config->rf_carrier_frequency_mhz >= 863 && usr_config->rf_carrier_frequency_mhz <= 870) &&
        !(usr_config->rf_carrier_frequency_mhz >= 902 && usr_config->rf_carrier_frequency_mhz <= 928))
        return false; 
#endif
 
#if defined (LORA_SX127X)
    if (!(usr_config->spreading_factor >= 6 && usr_config->spreading_factor <= 12))
        return false;
#elif defined (LORA_SX126X)
    if (!(usr_config->spreading_factor >= 5 && usr_config->spreading_factor <= 12))
        return false;
#endif

#if defined (LORA_SX127X)
    if (!(usr_config->tx_timeout_ms >= 1 && usr_config->tx_timeout_ms <= 0xFFFFFFFF))
        return false;
#elif defined (LORA_SX126X)
    if (!(usr_config->tx_timeout_ms >= 1 && usr_config->tx_timeout_ms <= 262000))
        return false;
#endif

    if (usr_config->recv_mode == LORA_SINGLE_RECEPTION)
    {
#if defined (LORA_SX127X)
        if (!(usr_config->rx_timeout_ms >= 3 && usr_config->rx_timeout_ms <= 8392))
            return false;
#elif defined (LORA_SX126X)
        if (!(usr_config->rx_timeout_ms >= 1 && usr_config->rx_timeout_ms <= 262000))
            return false;
#endif
    }
    if (usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION)
    {
#if defined (LORA_SX127X)
        if (!(usr_config->rx_timeout_ms >= 1 && usr_config->rx_timeout_ms <= 0xFFFFFFFF))
            return false;
#elif defined (LORA_SX126X)
        if (!(usr_config->rx_timeout_ms >= 1 && usr_config->rx_timeout_ms <= 262000))
            return false;
#endif
    }

    if (usr_config->txrx == LORA_TX && !(usr_config->payload_length <= usr_config->max_payload_length))
        return false;
    else if (usr_config->txrx == LORA_RX &&
            ((usr_config->implicit_header_mode_on==1 && !(usr_config->payload_length != 0)) || 
             (usr_config->implicit_header_mode_on==0 && !(usr_config->payload_length <= usr_config->max_payload_length))))
        return false;

    if (!(usr_config->max_payload_length >= 1 && usr_config->max_payload_length <= LORA_MAX_PAYLOAD_LEN))
        return false;

#if defined (LORA_SX127X)
    if (usr_config->chip == LORA_CHIP_SX1272 &&
       !(usr_config->signal_bandwidth >= 0 && usr_config->signal_bandwidth <= 2))
        return false;
    else if (usr_config->chip == LORA_CHIP_SX1276 &&
            !(usr_config->signal_bandwidth >= 0 && usr_config->signal_bandwidth <= 9))
        return false;
#elif defined (LORA_SX126X)
        if (!(usr_config->signal_bandwidth >= 0 && usr_config->signal_bandwidth <= 6) && 
            !(usr_config->signal_bandwidth >= 8 && usr_config->signal_bandwidth <= 10))
            return false;
#endif
    if (!(usr_config->enable_high_power_pa == 0 || usr_config->enable_high_power_pa == 1))
        return false;
#endif
    return true;
}

static void lora_open_s(LORA* lora, HANDLE process, IO* io_config)
{
    SPI_MODE mode;
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    uint32_t toa_ms;

    if (!io_config)
    {
        error(ERROR_NOT_CONFIGURED);
        return; 
    }
    memcpy(usr_config, io_data(io_config), sizeof(LORA_USR_CONFIG));

    if (!lora_usr_config_valid(lora))
    {
#if (LORA_DEBUG)
        lora_set_config_error(lora, __FILE__, __LINE__, 0);
#endif
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    if (!sx12xx_config(lora))
    {
#if (LORA_DEBUG)
        lora_set_config_error(lora, __FILE__, __LINE__, 0);
#endif
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    mode.cs_pin        = usr_config->spi_cs_pin;
    mode.cpha          = 0;
    mode.cpol          = 0;
    mode.size          = 8;     
    mode.io_mode       = 1;
    mode.order         = SPI_MSBFIRST;
    lora->spi.port     = usr_config->spi_port;
#if defined(STM32)
    SPI_SPEED speed;
    unsigned int bus_clock, div;
    bus_clock = power_get_clock(POWER_BUS_CLOCK);
    for (speed = SPI_DIV_2; speed <= SPI_DIV_256; ++speed)
    {
        div = (1 << (speed+1));
        if ((bus_clock / div) <= (usr_config->spi_baudrate_mbps * 1000000))
            break;
    }
    mode.speed         = speed;
#elif defined (MK22)
    mode.baudrate_mbps = usr_config->spi_baudrate_mbps;
    // if io mode then control cs externally (iio_complete will notify os that spi transfer is competed)
    // note: ctrl_cs can be removed (if need)
    //mode.ctrl_cs       = !mode.io_mode;  
#endif
    spi_open(lora->spi.port, &mode);

    lora->process = process;
    lora->rxcont_mode_started = false;
    lora->io_rx = 0;
    lora_clear_stats_s(lora);

    lora->tx_on = false;
    lora->rx_on = false;
    if (usr_config->txrx == LORA_TX || usr_config->txrx == LORA_TXRX) lora->tx_on = true;    
    if (usr_config->txrx == LORA_RX || usr_config->txrx == LORA_TXRX) lora->rx_on = true;

#if defined (LORA_SX126X)
    /* The BUSY control line is used to indicate the status of the internal 
       state machine. When the BUSY line is held low, it indicates that the 
       internal state machine is in idle mode and that the radio is ready to 
       accept a command from the host controller. */
    gpio_enable_pin(usr_config->busy_pin, GPIO_MODE_IN_PULLDOWN);
    gpio_reset_pin(usr_config->busy_pin); 
    pin_enable(usr_config->busy_pin, usr_config->busy_pin_mode, false);
#endif
    
#if defined (LORA_SX127X)
    sx127x_open(lora);
#elif defined (LORA_SX126X)
    sx126x_open(lora);
#endif

    /* NOTE 1: Set reasonable timeout values
       NOTE 2: Usually LORA_POLL_TIMEOUT_MS < pkt_time_on_air_ms < tx_timeout_ms/rx_timeout_ms 
               Example                10 ms              1200 ms         5000 ms       5000 ms */
    toa_ms = (uint32_t)lora_get_stat(lora, PKT_TIME_ON_AIR_MS);
    if (!(LORA_POLL_TIMEOUT_MS < toa_ms && toa_ms < usr_config->tx_timeout_ms && 
                                           toa_ms < usr_config->rx_timeout_ms))
    {
#if (LORA_DEBUG)
        lora_set_config_error(lora, __FILE__, __LINE__, 1, toa_ms);
#endif
		error(ERROR_NOT_CONFIGURED);
        return;
    }

#if (LORA_DEBUG) && (LORA_DEBUG_INFO)
    /* [implicit_header_mode_on] ImplicitHeaderModeOn:
            0 - Explicit Header mode
            1 - Implicit Header mode */
    if (usr_config->txrx == LORA_TX)
        printf("[loras] [info] init completed: TX in %s header mode\n",
            usr_config->implicit_header_mode_on == 1? "IMPLICIT" : "EXPLICIT");
    else if (usr_config->txrx == LORA_RX)
        printf("[loras] [info] init completed: RX in %s header mode with %s reception\n",
            usr_config->implicit_header_mode_on == 1? "IMPLICIT" : "EXPLICIT", 
            usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION ? "CONTINUOUS" : "SINGLE");        
    else if (usr_config->txrx == LORA_TXRX)
        printf("[loras] [info] init completed: TX-RX in %s header mode with %s reception\n",
            usr_config->implicit_header_mode_on == 1? "IMPLICIT" : "EXPLICIT", 
            usr_config->recv_mode == LORA_CONTINUOUS_RECEPTION ? "CONTINUOUS" : "SINGLE"); 
#endif

    lora->error  = LORA_ERROR_NONE;
    lora->req    = LORA_REQ_NONE;
    lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_IDLE), (unsigned int)lora->error, 0, 0);
}

static void lora_close_s(LORA* lora)
{
    //stop calling lora_timer_poll_timeout
    timer_stop(lora->timer_poll_timeout, 0, HAL_LORA);
    lora->timer_poll_timeout_stopped = true;

    timer_stop(lora->timer_txrx_timeout, 0, HAL_LORA);

#if defined (LORA_SX127X)
    sx127x_close(lora);
#elif defined (LORA_SX126X)
    sx126x_close(lora);
#endif

    spi_close(lora->spi.port);
#if defined (LORA_SX126X)
    LORA_USR_CONFIG* usr_config = &lora->usr_config;
    /* The BUSY control line is used to indicate the status of the internal 
       state machine. When the BUSY line is held low, it indicates that the 
       internal state machine is in idle mode and that the radio is ready to 
       accept a command from the host controller. */
    gpio_disable_pin(usr_config->busy_pin);
    pin_disable(usr_config->busy_pin);
#endif
}

static inline void lora_txrx_async_s(LORA* lora, IO* io, bool is_tx)
{
    int32_t timeout_ms;

#if defined (LORA_SX127X)
    (is_tx ? sx127x_tx_async : sx127x_rx_async)(lora, io);
#elif defined (LORA_SX126X)
    (is_tx ? sx126x_tx_async : sx126x_rx_async)(lora, io);
#endif

    if (is_tx)
    {
        lora->req = LORA_REQ_SEND;
        timeout_ms = (uint32_t)lora_get_stat(lora, PKT_TIME_ON_AIR_MS);
    }
    else
    {
        lora->req = LORA_REQ_RECV;
        timeout_ms = LORA_POLL_TIMEOUT_MS; 
    }

    timer_start_ms(lora->timer_poll_timeout, timeout_ms);
    lora->timer_poll_timeout_stopped = false;
#if (LORA_DEBUG)
    lora->transfer_in_progress_ms = lora_get_uptime_ms();
    lora->polls_cnt = 0;
#endif
}

static inline void lora_tx_async_s(LORA* lora, IO* io)
{
    lora_txrx_async_s(lora, io, true);
}

static inline void lora_rx_async_s(LORA* lora, IO* io)
{
    lora_txrx_async_s(lora, io, false);
}

static inline void lora_timer_poll_timeout(LORA* lora)
{
#if (LORA_DEBUG)
    ++lora->polls_cnt;
#endif

#if (LORA_DEBUG)
    //todo: os timers issue
    if (lora->timer_poll_timeout_stopped || lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        lora_set_intern_error(lora, __FILE__, __LINE__, 1, lora->status);//EXTRA_STATUS_CHECK
        return;
    }
#endif

#if defined (LORA_SX127X)
    if      (lora->tx_on && lora->req == LORA_REQ_SEND) sx127x_tx_async_wait(lora);
    else if (lora->rx_on && lora->req == LORA_REQ_RECV) sx127x_rx_async_wait(lora);
#elif defined (LORA_SX126X)  
    if      (lora->tx_on && lora->req == LORA_REQ_SEND) sx126x_tx_async_wait(lora);
    else if (lora->rx_on && lora->req == LORA_REQ_RECV) sx126x_rx_async_wait(lora);
#endif 

    if (lora->status == LORA_STATUS_TRANSFER_IN_PROGRESS) 
    {
        timer_start_ms(lora->timer_poll_timeout, LORA_POLL_TIMEOUT_MS);
        ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_TRANSFER_IN_PROGRESS), 0, 0, 0);
    }
    else if (lora->status == LORA_STATUS_TRANSFER_COMPLETED)
    {
        timer_stop(lora->timer_txrx_timeout, 0, HAL_LORA);
#if (LORA_DEBUG)
        uint32_t duration_ms = lora_get_uptime_ms() - lora->transfer_in_progress_ms;
        printf("[loras] [info] poll completed duration_ms:%u polls_cnt:%u\n", duration_ms, lora->polls_cnt);
#endif
        ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_TRANSFER_COMPLETED), lora->error, 0, 0);
    }
}

static inline void lora_get_stats_s(LORA* lora, LORA_STATS *stats)
{
    if (lora->tx_on || lora->rx_on)
    {
        stats->pkt_time_on_air_ms      = (uint32_t)lora_get_stat(lora, PKT_TIME_ON_AIR_MS);
    }
    if (lora->tx_on)
    {
        stats->tx_timeout_num          = lora->stats.tx_timeout_num;
        stats->tx_output_power_dbm     = (  int8_t)lora_get_stat(lora, TX_OUTPUT_POWER_DBM);
    }
    if (lora->rx_on) 
    {
        stats->rx_timeout_num          = lora->stats.rx_timeout_num;
        stats->crc_err_num             = lora->stats.crc_err_num;
        stats->rssi_cur_dbm            = ( int16_t)lora_get_stat(lora, RSSI_CUR_DBM);
        stats->rssi_last_pkt_rcvd_dbm  = ( int16_t)lora_get_stat(lora, RSSI_LAST_PKT_RCVD_DBM);
        stats->snr_last_pkt_rcvd_db    = (  int8_t)lora_get_stat(lora, SNR_LAST_PKT_RCVD_DB);
        stats->rf_freq_error_hz        = ( int32_t)lora_get_stat(lora, RF_FREQ_ERROR_HZ);
    }   
}

static void lora_abort_rx_transfer_s(LORA* lora)
{
    if (lora->rx_on && lora->status == LORA_STATUS_TRANSFER_IN_PROGRESS )
    {
        timer_stop(lora->timer_txrx_timeout, 0, HAL_LORA);
        timer_stop(lora->timer_poll_timeout, 0, HAL_LORA);
        lora->timer_poll_timeout_stopped = true;
        lora_set_error(lora, LORA_ERROR_RX_TRANSFER_ABORTED);
        ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_TRANSFER_COMPLETED), lora->error, 0, 0);
    }
}

static void lora_request(LORA* lora, IPC* ipc)
{
    if (lora->status == LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_TIMEOUT:
        case LORA_ABORT_RX_TRANSFER:
            break;
        default:
#if (LORA_DEBUG)
            printf("[loras] [api usage error] new req while transfer in progress (req discarded)\n");
#endif
            return;
        }        
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lora_open_s(lora, ipc->process, (IO*)ipc->param1);
        break;
    case IPC_CLOSE:
        lora_close_s(lora);
        break;
    case IPC_WRITE:
        lora_tx_async_s(lora, (IO*)ipc->param1);
        break;
    case IPC_READ:
        lora_rx_async_s(lora, (IO*)ipc->param1);
        break;   
    case LORA_GET_STATS:
        lora_get_stats_s(lora, (LORA_STATS*)ipc->param1);
        break;  
    case LORA_CLEAR_STATS:
        lora_clear_stats_s(lora);
        break;  
    case LORA_ABORT_RX_TRANSFER:
        lora_abort_rx_transfer_s(lora);
        break;  
    case IPC_TIMEOUT:
        switch (ipc->param1)
        {
        case LORA_TIMER_TXRX_TIMEOUT_ID: 
            lora_timer_txrx_timeout(lora);
            break;
        case LORA_TIMER_POLL_TIMEOUT_ID: 
            lora_timer_poll_timeout(lora);
            break;
        default:
            error (ERROR_NOT_SUPPORTED);
            break;
        }
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        break;
    }
}
#if 0
static void spi_request(LORA* lora, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
#if (LORA_DEBUG)
        printf("%s\n", "spi_request: IPC_READ");
#endif
        break;
    case IPC_WRITE:
#if (LORA_DEBUG)
        printf("%s\n", "spi_request: IPC_WRITE");
#endif
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        break;
    }
}
#endif
static void init(LORA* lora)
{
    memset(lora, 0, sizeof(LORA));
    lora->spi.io = io_create(LORA_SPI_IO_SIZE);
    lora->timer_txrx_timeout = timer_create(LORA_TIMER_TXRX_TIMEOUT_ID, HAL_LORA);
    lora->timer_poll_timeout = timer_create(LORA_TIMER_POLL_TIMEOUT_ID, HAL_LORA);
    lora->por_reset_completed = false;
}

#else // #if defined (LORA_SX127X) || defined (LORA_SX126X)

static void init(LORA* lora)
{
    memset(lora, 0, sizeof(LORA));
}

#endif

void loras_main()
{
    IPC ipc;
    LORA lora;
#if (LORA_DEBUG)
    open_stdout();
#endif //LORA_DEBUG
    init(&lora);

    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_LORA:
#if defined (LORA_SX127X) || defined (LORA_SX126X)
            lora_request(&lora, &ipc);
#endif
            break;
        //todo: do we need to receive and process HAL_SPI reqs (ex. from iio_complete)??
        //case HAL_SPI:
        //    spi_request(&lora, &ipc);
        //    break;
        default:
            error (ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}