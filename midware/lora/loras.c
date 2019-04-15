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
#include "power.h"

#include <string.h>

#if (LORA_DEBUG)
static uint32_t loras_get_uptime_ms()
{
    SYSTIME uptime;
    uint32_t ms_passed;
    get_uptime(&uptime);
    ms_passed = (uptime.sec * 1000) + (uptime.usec / 1000);
    return ms_passed;
}
#endif

static void loras_get_stats(LORA* lora, IO* io_stats_tx, IO* io_stats_rx)
{
    LORA_STATS_TX *stats_tx;
    LORA_STATS_RX *stats_rx;

    if (io_stats_tx)
    {
        stats_tx = (LORA_STATS_TX*)io_data(io_stats_tx);
        stats_tx->pkt_time_on_air_ms      = (uint32_t)loras_hw_get_stat(lora, PKT_TIME_ON_AIR_MS);
        stats_tx->timeout_num             = lora->stats_tx.timeout_num;
        stats_tx->output_power_dbm        = (  int8_t)loras_hw_get_stat(lora, TX_OUTPUT_POWER_DBM);
    }
    if (io_stats_rx)
    {
        stats_rx = (LORA_STATS_RX*)io_data(io_stats_rx);
        stats_rx->pkt_time_on_air_ms      = (uint32_t)loras_hw_get_stat(lora, PKT_TIME_ON_AIR_MS);
        stats_rx->timeout_num             = lora->stats_rx.timeout_num;
        stats_rx->crc_err_num             = lora->stats_rx.crc_err_num;
        stats_rx->rssi_cur_dbm            = ( int16_t)loras_hw_get_stat(lora, RX_RSSI_CUR_DBM);
        stats_rx->rssi_last_pkt_rcvd_dbm  = ( int16_t)loras_hw_get_stat(lora, RX_RSSI_LAST_PKT_RCVD_DBM);
        stats_rx->snr_last_pkt_rcvd_db    = (  int8_t)loras_hw_get_stat(lora, RX_SNR_LAST_PKT_RCVD_DB);
        stats_rx->rf_freq_error_hz        = ( int32_t)loras_hw_get_stat(lora, RX_RF_FREQ_ERROR_HZ);
    }
}

static void loras_clear_stats(LORA* lora)
{
    memset(&lora->stats_tx, 0, sizeof(LORA_STATS_TX));
    memset(&lora->stats_rx, 0, sizeof(LORA_STATS_RX));
}

static bool loras_spi_open(LORA* lora)
{
    SPI_MODE mode;
    LORA_CONFIG *config = &lora->config;
    mode.cs_pin        = config->spi_cs_pin;
    mode.cpha          = 0;
    mode.cpol          = 0;
    mode.size          = 8;
    mode.io_mode       = 1;
    mode.order         = SPI_MSBFIRST;
    lora->spi.port     = config->spi_port;
#if defined(STM32)
    SPI_SPEED speed;
    unsigned int bus_clock, div;
    bus_clock = power_get_clock(POWER_BUS_CLOCK);
    bool found = false;
    for (speed = SPI_DIV_2; speed <= SPI_DIV_256; ++speed)
    {
        div = (1 << (speed+1));
        if ((bus_clock / div) <= (LORA_SCK_FREQUENCY_MHZ * 1000000))
        {
            found = true;
            break;
        }
    }
    if (!found) return false;
    mode.speed = speed;
#elif defined (MK22)
    mode.baudrate_mbps = LORA_SCK_FREQUENCY_MHZ;
    //todo: if io mode then control cs externally (iio_complete will notify os that spi transfer is competed)
    //note: ctrl_cs can be removed (if need)
    //mode.ctrl_cs       = !mode.io_mode;
#else
#error unsupported
#endif
    spi_open(lora->spi.port, &mode);
    return true;
}

static void loras_spi_close(LORA* lora)
{
    spi_close(lora->spi.port);
}

static void loras_clear_vars(LORA* lora)
{
    lora->io = NULL;
    lora->rxcont_mode_started = false;
    lora->busy_pin_state_invalid = false;
    lora->opened = false;
    lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    loras_clear_stats(lora);
}

static void loras_close(LORA* lora)
{
    timer_stop(lora->timer_poll_timeout, 0, HAL_LORA);
    timer_stop(lora->timer_txrx_timeout, 0, HAL_LORA);
    loras_hw_close(lora);
    loras_spi_close(lora);
    loras_clear_vars(lora);
}

static void loras_open(LORA* lora, HANDLE process, IO* io_config)
{
    LORA_CONFIG* config = &lora->config;
    if (!(io_config && process != INVALID_HANDLE))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    lora->process = process;
    memcpy(config, io_data(io_config), sizeof(LORA_CONFIG));

    if (!loras_spi_open(lora))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }

    loras_clear_vars(lora);

    if (!loras_hw_open(lora))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    lora->opened = true;

#if (LORA_DEBUG)
    printf("[loras] [info] open completed\n");
#endif
}

static bool loras_payload_length_valid(LORA* lora)
{
    uint8_t payload_length = lora->io->data_size;

    if (lora->tx || LORA_IMPLICIT_HEADER_MODE_ON)
    {
        /* A 0 value is not permitted */
        if (!(payload_length >= 1 && payload_length <= LORA_MAX_PAYLOAD_LEN))
            return false;
    }
    else if (!LORA_IMPLICIT_HEADER_MODE_ON)
    {
        /* A 0 value is permitted (payload_length will be obtained from the header) */
        if (!(payload_length >= 0 && payload_length <= LORA_MAX_PAYLOAD_LEN))
            return false;
    }
    return true;
}

static bool loras_autoset_txrx_timeout_ms(LORA* lora)
{
    uint32_t toa_cur, toa_max, toa, aux;
    /* NOTE 1: Set reasonable timeout values
       NOTE 2: Usually LORA_POLL_TIMEOUT_MS < pkt_time_on_air_ms < tx_timeout_ms/rx_timeout_ms
               Example                10 ms              1200 ms         5000 ms       5000 ms */
    toa_cur = (uint32_t)loras_hw_get_stat(lora, PKT_TIME_ON_AIR_MS);
    toa_max = (uint32_t)loras_hw_get_stat(lora, PKT_TIME_ON_AIR_MAX_MS);
    if (!(toa_cur > 0 && toa_max > 0 && toa_cur > LORA_POLL_TIMEOUT_MS && toa_max >= toa_cur))
    {
        return false;
    }
    /* TX:      pl is     known before tx_async => get time_on_air_ms     (based on pl)
       RX (ih): pl is     known before rx_async => get time_on_air_ms     (based on pl)
       RX (eh): pl is not known before rx_async => get time_on_air_max_ms (based on pl_max) */
    if (lora->tx)
    {
        toa = toa_cur;
        aux = (toa * LORA_AUTO_TIMEOUT_ADDITIVE_PERCENTAGE) / 100;
        /* chip     timer type    min value    max value
           sx127x   external      1 ms         max value supported by timer_start_ms
           sx126x   internal      1 ms         262000 ms */
        //note: tx_timeout_ms will be auto trimmed (if need) then
        lora->tx_timeout_ms = toa + (aux ? aux : 1);
    }
    else
    {
        toa = LORA_IMPLICIT_HEADER_MODE_ON ? toa_cur : toa_max;
        aux = (toa * LORA_AUTO_TIMEOUT_ADDITIVE_PERCENTAGE) / 100;
        /* == LORA_SINGLE_RECEPTION ================================================
           chip     timer type    min value    max value
           sx127x   internal      3 ms [1]     8392   ms [2]
           sx126x   internal      1 ms         262000 ms
           sx127x notes:
             - rx_timeout_ms will be auto-converted to rx_timeout_symb (4..1023 symbols)
             - min/max values depend on bw (125/250/500 kHz) and sf (6..12)
           [1] min value calculated for min bw (125 kHz) and min sf (6)  => leads to 4    symbols
           [2] max value calculated for max bw (500 kHz) and max sf (12) => leads to 1023 symbols
           == LORA_CONTINUOUS_RECEPTION ============================================
           chip     timer type    min value    max value
           sx127x   external      1 ms         max value supported by timer_start_ms
           sx126x   external      1 ms         max value supported by timer_start_ms */
        //note: rx_timeout_ms will be auto trimmed (if need) then
        lora->rx_timeout_ms = toa + (aux ? aux : 1);
    }
    return true;
}

static void loras_fatal(LORA* lora)
{
    error(ERROR_INTERNAL);
    loras_close(lora);
}

static void loras_txrx_async(LORA* lora, IO* io)
{
    int32_t timeout_ms;
#if (LORA_DEBUG)
    int last_error;
#endif
    //save io (will be used in get_payload_length)
    lora->io = io;

    if (!loras_payload_length_valid(lora))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }

    if (!loras_autoset_txrx_timeout_ms(lora))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }

    if (lora->tx)
    {
        loras_hw_tx_async(lora, io);
        timeout_ms = (uint32_t)loras_hw_get_stat(lora, PKT_TIME_ON_AIR_MS);
    }
    else
    {
        loras_hw_rx_async(lora, io);
        timeout_ms = LORA_POLL_TIMEOUT_MS;
    }

    last_error = get_last_error();
    if (last_error != ERROR_OK)
    {
        loras_fatal(lora);
#if (LORA_DEBUG)
        printf("[loras] [fatal error] unexpected error %d => server closed\n", last_error);
#endif
        return;
    }

    timer_start_ms(lora->timer_poll_timeout, timeout_ms);
#if (LORA_DEBUG)
    lora->transfer_in_progress_ms = loras_get_uptime_ms();
    lora->polls_cnt = 0;
#endif
}

static void loras_tx_async(LORA* lora, IO* io)
{
    lora->tx = true;
    loras_txrx_async(lora, io);
}

static void loras_rx_async(LORA* lora, IO* io)
{
    lora->tx = false;
    loras_txrx_async(lora, io);
}

static void loras_timer_txrx_timeout(LORA* lora)
{
    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        loras_fatal(lora);
#if (LORA_DEBUG)
        printf("[loras] [fatal error] unexpected status %d => server closed\n", lora->status);
#endif
        return;
    }
#if (LORA_DEBUG)
    printf("[loras] [info] timer_txrx_timeout expired -> transfer completed\n");
#endif
    timer_stop(lora->timer_poll_timeout, 0, HAL_LORA);
    if (lora->tx || !LORA_CONTINUOUS_RECEPTION_ON)
        loras_hw_sleep(lora);
#if (LORA_DO_ERROR_COUNTING)
    lora->tx ? ++lora->stats_tx.timeout_num : ++lora->stats_rx.timeout_num;
#endif
    lora->status = LORA_STATUS_TRANSFER_COMPLETED;
    ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_TRANSFER_COMPLETED), 0, 0, ERROR_TIMEOUT);
}

static void loras_timer_poll_timeout(LORA* lora)
{
    int last_error;
#if (LORA_DEBUG)
    uint32_t duration_ms;
    ++lora->polls_cnt;
#endif

    if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
    {
        loras_fatal(lora);
#if (LORA_DEBUG)
        printf("[loras] [fatal error] unexpected status %d => server closed\n", lora->status);
#endif
        return;
    }

    (lora->tx ? loras_hw_tx_async_wait : loras_hw_rx_async_wait)(lora);

    last_error = get_last_error();

#if (LORA_DEBUG)
    //check for internal unexpected errors (ex. ERROR_INVALID_STATE -- chip is in invalid state)
    if (!(last_error >= ERROR_OK || last_error == ERROR_TIMEOUT))
    {
        loras_fatal(lora);
        printf("[loras] [fatal error] unexpected error %d => server closed\n", last_error);
        return;
    }
#endif

    switch (lora->status)
    {
    case LORA_STATUS_TRANSFER_IN_PROGRESS:
        timer_start_ms(lora->timer_poll_timeout, LORA_POLL_TIMEOUT_MS);
        //note: do not check if last_error == ERROR_OK (should be)
        ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_TRANSFER_IN_PROGRESS), 0, 0, last_error);
        break;
    case LORA_STATUS_TRANSFER_COMPLETED:
        timer_stop(lora->timer_txrx_timeout, 0, HAL_LORA);
#if (LORA_DEBUG)
        duration_ms = loras_get_uptime_ms() - lora->transfer_in_progress_ms;
        printf("[loras] [info] poll completed duration_ms:%u polls_cnt:%u\n", duration_ms, lora->polls_cnt);
#endif
        ipc_post_inline(lora->process, HAL_CMD(HAL_LORA, LORA_TRANSFER_COMPLETED), 0, 0, last_error);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static void loras_abort_rx_transfer(LORA* lora)
{
    timer_stop(lora->timer_txrx_timeout, 0, HAL_LORA);
    timer_stop(lora->timer_poll_timeout, 0, HAL_LORA);
    if (!LORA_CONTINUOUS_RECEPTION_ON)
        loras_hw_sleep(lora);
    lora->status = LORA_STATUS_TRANSFER_COMPLETED;
}

static bool loras_request_check_pre(LORA* lora, IPC* ipc)
{
    if (!lora->opened && (HAL_ITEM(ipc->cmd) != IPC_OPEN))
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        if (lora->opened)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return false;
        }
        break;
    case IPC_CLOSE:
        //tbd
        break;
    case IPC_WRITE:
    case IPC_READ:
    case LORA_GET_STATS:
    case LORA_CLEAR_STATS:
        if (lora->status == LORA_STATUS_TRANSFER_IN_PROGRESS)
        {
            error(ERROR_INVALID_STATE);
            return false;
        }
        break;
    case IPC_CANCEL_IO:
        break;
    case IPC_TIMEOUT:
        switch (ipc->param1)
        {
        case LORA_TIMER_TXRX_TIMEOUT_ID:
        //todo: os timers issue
        case LORA_TIMER_POLL_TIMEOUT_ID:
            if (lora->status != LORA_STATUS_TRANSFER_IN_PROGRESS)
            {
                error(ERROR_INVALID_STATE);
                return false;
            }
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
    return true;
}

static bool loras_request_check_post(LORA* lora, IPC* ipc)
{
    if (LORA_CHIP == SX1261 && lora->busy_pin_state_invalid)
    {
        //todo: who should reboot the server: server itself (auto reboot) or app??
        error(ERROR_HARDWARE);
        return false;
    }
    return true;
}

static void loras_request(LORA* lora, IPC* ipc)
{
    if (!loras_request_check_pre(lora, ipc))
    {
        return;
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        loras_open(lora, ipc->process, (IO*)ipc->param1);
        break;
    case IPC_CLOSE:
        loras_close(lora);
        break;
    case IPC_WRITE:
        loras_tx_async(lora, (IO*)ipc->param1);
        break;
    case IPC_READ:
        loras_rx_async(lora, (IO*)ipc->param1);
        break;
    case LORA_GET_STATS:
        loras_get_stats(lora, (IO*)ipc->param1, (IO*)ipc->param2);
        break;
    case LORA_CLEAR_STATS:
        loras_clear_stats(lora);
        break;
    case IPC_CANCEL_IO:
        loras_abort_rx_transfer(lora);
        break;
    case IPC_TIMEOUT:
        switch (ipc->param1)
        {
        case LORA_TIMER_TXRX_TIMEOUT_ID:
            loras_timer_txrx_timeout(lora);
            break;
        case LORA_TIMER_POLL_TIMEOUT_ID:
            loras_timer_poll_timeout(lora);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }

    if (!loras_request_check_post(lora, ipc))
    {
        return;
    }
}

static void init(LORA* lora)
{
    memset(lora, 0, sizeof(LORA));
    lora->spi.io = io_create(LORA_SPI_IO_SIZE);
    lora->timer_txrx_timeout = timer_create(LORA_TIMER_TXRX_TIMEOUT_ID, HAL_LORA);
    lora->timer_poll_timeout = timer_create(LORA_TIMER_POLL_TIMEOUT_ID, HAL_LORA);
    lora->opened = false;
    lora->sys_vars = NULL;
}

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
            loras_request(&lora, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}