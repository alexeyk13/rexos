/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_uart.h"
#include "nrf_power.h"
#include "../../userspace/sys.h"
#include "../kerror.h"
#include "../kstdlib.h"
#include "../ksystime.h"
#include "../kstream.h"
#include "../kirq.h"
#include "../kexo.h"
#include <string.h>
#include "nrf_exo_private.h"
#include "sys_config.h"

typedef NRF_UART_Type*      NRF_UART_Type_P;

#if defined(NRF51)
static const unsigned int UART_VECTORS[UARTS_COUNT] = {UART0_IRQn};
static const NRF_UART_Type_P UART_REGS[UARTS_COUNT] = {NRF_UART0};
#endif // NRF51

static inline bool nrf_uart_on_tx_isr(EXO* exo, UART_PORT port)
{
    UART* uart = exo->uart.uarts[port];
#if (UART_IO_MODE_SUPPORT)
    if (uart->io_mode)
    {
        if (uart->i.tx_io)
        {
            //TXC(port, ((uint8_t*)io_data(uart->i.tx_io))[uart->i.tx_processed++]);
            if (uart->i.tx_processed < uart->i.tx_io->data_size)
                return true;
            iio_complete(uart->i.tx_process, HAL_IO_CMD(HAL_UART, IPC_WRITE), port, uart->i.tx_io);
            uart->i.tx_io = NULL;
        }
    }
    else
#endif //UART_IO_MODE_SUPPORT
    {
        if (uart->s.tx_size == 0)
        {
            uart->s.tx_total = uart->s.tx_size = stream_iread_no_block(uart->s.tx_handle, uart->s.tx_buf, UART_BUF_SIZE);
            //nothing more, listen
            if (uart->s.tx_size == 0)
            {
                UART_REGS[port]->INTENSET &= ~UART_INTENSET_TXDRDY_Msk;
                stream_ilisten(uart->s.tx_stream, port, HAL_UART);
                return false;
            }
        }
        UART_REGS[port]->TXD = uart->s.tx_buf[uart->s.tx_total - uart->s.tx_size--];
        return  true;
    }
    return false;
}

static inline void nrf_uart_on_rx_isr(EXO* exo, UART_PORT port, uint8_t c)
{
#if (UART_IO_MODE_SUPPORT)
    UART* uart = exo->uart.uarts[port];
    if (uart->io_mode)
    {
#if (STM32_UART_DISABLE_ECHO)
        if (UART_REGS[port]->CR1 & USART_CR1_TE)
            return;
#endif //STM32_UART_DISABLE_ECHO
        timer_istop(uart->i.rx_timer);
        if (uart->i.rx_io)
        {
            ((uint8_t*)io_data(uart->i.rx_io))[uart->i.rx_io->data_size++] = c;
            //max size limit
            if (uart->i.rx_io->data_size >= uart->i.rx_max)
            {
                iio_complete(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, uart->i.rx_io);
#if (UART_DOUBLE_BUFFERING)
                if(uart->i.rx2_io)
                {
                    uart->i.rx_io = uart->i.rx2_io;
                    uart->i.rx_max = uart->i.rx2_max;
                    uart->i.rx2_io = NULL;
                    timer_istart_us(uart->i.rx_timer, uart->i.rx_interleaved_timeout);
                }else
#endif // UART_DOUBLE_BUFFERING
                {
                    uart->i.rx_io = NULL;
                }
            }
            else
                timer_istart_us(uart->i.rx_timer, uart->i.rx_interleaved_timeout);
        }else{
            exo->uart.uarts[port]->error = ERROR_CHAR_LOSS;
        }
    }
    else
#endif //UART_IO_MODE_SUPPORT
    if (!stream_iwrite_no_block(exo->uart.uarts[port]->s.rx_handle, (char*)&c, 1))
        exo->uart.uarts[port]->error = ERROR_CHAR_LOSS;
}

void nrf_uart_on_isr(int vector, void* param)
{
    //find port by vector
    UART_PORT port = UART_0;
    EXO* exo = (EXO*)param;
    uint32_t sr = UART_REGS[port]->ERRORSRC;
    // tx
    if (UART_REGS[port]->EVENTS_TXDRDY != 0)
    {
        // clear UART TX event flag
        UART_REGS[port]->EVENTS_TXDRDY = 0;
        nrf_uart_on_tx_isr(exo, port);
    }

    if (UART_REGS[port]->EVENTS_CTS || UART_REGS[port]->EVENTS_NCTS)
    {
        UART_REGS[port]->EVENTS_CTS = 0;
        UART_REGS[port]->EVENTS_NCTS = 0;
    }

    if(UART_REGS[port]->EVENTS_RXTO != 0)
        UART_REGS[port]->EVENTS_RXTO = 0;

    //decode error, if any
    if (UART_REGS[port]->EVENTS_ERROR != 0)
    {
        // clear UART ERROR event flag
        UART_REGS[port]->EVENTS_ERROR = 0;
        if(sr & UART_ERRORSRC_BREAK_Msk)
            exo->uart.uarts[port]->error = ERROR_COMM_BREAK;
        else if(sr & UART_ERRORSRC_OVERRUN_Msk)
            exo->uart.uarts[port]->error = ERROR_COMM;
        else if(sr & UART_ERRORSRC_FRAMING_Msk)
            exo->uart.uarts[port]->error = ERROR_INVALID_FRAME;
        else if(sr & UART_ERRORSRC_PARITY_Msk)
            exo->uart.uarts[port]->error = ERROR_INVALID_PARITY;
    }
    // rx
    if (UART_REGS[port]->EVENTS_RXDRDY != 0)
    {
        // clear UART RX event flag
        UART_REGS[port]->EVENTS_RXDRDY = 0;
        nrf_uart_on_rx_isr(exo, port, UART_REGS[port]->RXD);
    }
}

void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
    NVIC_DisableIRQ(UART_VECTORS[(UART_PORT)param]);
    int i;
    // disable uart irq
    UART_REGS[(UART_PORT)param]->INTENCLR |= UART_INTENCLR_TXDRDY_Msk;
    for(i = 0; i < size; ++i)
    {
        // send new data
        UART_REGS[(UART_PORT)param]->TXD = buf[i];
        // wait previous TX complete
        while(UART_REGS[(UART_PORT)param]->EVENTS_TXDRDY != 1);
        // clear flag
        UART_REGS[(UART_PORT)param]->EVENTS_TXDRDY = 0;
    }
    // enable uart irq
    UART_REGS[(UART_PORT)param]->INTENSET |= UART_INTENSET_TXDRDY_Msk;
    //transmitter will be disabled in next IRQ TC
    NVIC_EnableIRQ(UART_VECTORS[(UART_PORT)param]);
}

static inline void nrf_uart_set_baudrate(EXO* exo, UART_PORT port, IPC* ipc)
{
    BAUD baudrate;
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    uart_decode_baudrate(ipc, &baudrate);

    switch(baudrate.baud)
    {
        case 115200:
            UART_REGS[port]->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud115200 << UART_BAUDRATE_BAUDRATE_Pos);
            break;
        default:
            UART_REGS[port]->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud9600 << UART_BAUDRATE_BAUDRATE_Pos);
            break;
    }

    if('N' != baudrate.parity)
        UART_REGS[port]->CONFIG |= UART_CONFIG_PARITY_Msk;
}

static void nrf_uart_flush_internal(EXO* exo, UART_PORT port)
{
#if (UART_IO_MODE_SUPPORT)
    if (exo->uart.uarts[port]->io_mode)
    {
        IO* rx_io;
        IO* tx_io;
        __disable_irq();
        rx_io = exo->uart.uarts[port]->i.rx_io;
        exo->uart.uarts[port]->i.rx_io = NULL;
        tx_io = exo->uart.uarts[port]->i.tx_io;
        exo->uart.uarts[port]->i.tx_io = NULL;
        __enable_irq();
        if (rx_io)
            kexo_io_ex(exo->uart.uarts[port]->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, rx_io, ERROR_IO_CANCELLED);
#if (UART_DOUBLE_BUFFERING)
        rx_io = exo->uart.uarts[port]->i.rx2_io;
        exo->uart.uarts[port]->i.rx2_io = NULL;
        if (rx_io)
            kexo_io_ex(exo->uart.uarts[port]->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, rx_io, ERROR_IO_CANCELLED);
#endif // UART_DOUBLE_BUFFERING

        if (tx_io)
            kexo_io_ex(exo->uart.uarts[port]->i.tx_process, HAL_IO_CMD(HAL_UART, IPC_WRITE), port, tx_io, ERROR_IO_CANCELLED);
        ksystime_soft_timer_stop(exo->uart.uarts[port]->i.rx_timer);
    }
    else
#endif //UART_IO_MODE_SUPPORT
    {
        if (exo->uart.uarts[port]->s.tx_stream != INVALID_HANDLE)
        {
            kstream_flush(exo->uart.uarts[port]->s.tx_stream);
            kstream_listen(KERNEL_HANDLE, exo->uart.uarts[port]->s.tx_stream, port, HAL_UART);
            __disable_irq();
            exo->uart.uarts[port]->s.tx_total = exo->uart.uarts[port]->s.tx_size = 0;
            __enable_irq();
        }
        if (exo->uart.uarts[port]->s.rx_stream != INVALID_HANDLE)
            kstream_flush(exo->uart.uarts[port]->s.rx_stream);
    }
    exo->uart.uarts[port]->error = ERROR_OK;
}

static inline bool nrf_uart_open_stream(UART* uart, UART_PORT port, unsigned int mode)
{
    uart->s.tx_stream = INVALID_HANDLE;
    uart->s.tx_handle = INVALID_HANDLE;
    uart->s.rx_stream = INVALID_HANDLE;
    uart->s.rx_handle = INVALID_HANDLE;
    uart->s.tx_total = uart->s.tx_size = 0;

    if (mode & UART_TX_STREAM)
    {
        if ((uart->s.tx_stream = kstream_create(UART_STREAM_SIZE)) == INVALID_HANDLE)
            return false;
        if ((uart->s.tx_handle = kstream_open(KERNEL_HANDLE, uart->s.tx_stream)) == INVALID_HANDLE)
            return false;
        kstream_listen(KERNEL_HANDLE, uart->s.tx_stream, port, HAL_UART);
    }
    if (mode & UART_RX_STREAM)
    {
        if ((uart->s.rx_stream = kstream_create(UART_STREAM_SIZE)) == INVALID_HANDLE)
            return false;
        if ((uart->s.rx_handle = kstream_open(KERNEL_HANDLE, uart->s.rx_stream)) == INVALID_HANDLE)
            return false;
        uart->s.rx_free = kstream_get_free(uart->s.rx_stream);
        //enable receiver
        // TODO ...
    }
    return true;
}

static void nrf_uart_destroy(EXO* exo, UART_PORT port)
{
#if (UART_IO_MODE_SUPPORT)
    if (exo->uart.uarts[port]->io_mode)
    {
        ksystime_soft_timer_destroy(exo->uart.uarts[port]->i.rx_timer);
    }
    else
#endif //UART_IO_MODE_SUPPORT
    {
        //rx
        kstream_close(KERNEL_HANDLE, exo->uart.uarts[port]->s.rx_handle);
        kstream_destroy(exo->uart.uarts[port]->s.rx_stream);
        //tx
        kstream_close(KERNEL_HANDLE, exo->uart.uarts[port]->s.tx_handle);
        kstream_destroy(exo->uart.uarts[port]->s.tx_stream);
    }

    kfree(exo->uart.uarts[port]);
    exo->uart.uarts[port] = NULL;

    // disable uart
    UART_REGS[port]->ENABLE       = (UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos);
    UART_REGS[port]->TASKS_STOPTX = 1;
    UART_REGS[port]->TASKS_STOPRX = 1;
    UART_REGS[port]->TASKS_SUSPEND = 1;
}

static inline void nrf_uart_open(EXO* exo, UART_PORT port, unsigned int mode)
{
    bool ok;
    if (exo->uart.uarts[port] != NULL)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    exo->uart.uarts[port] = kmalloc(sizeof(UART));
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_OUT_OF_MEMORY);
        return;
    }

    exo->uart.uarts[port]->error = ERROR_OK;
    exo->uart.uarts[port]->io_mode = ((mode & UART_MODE) == UART_MODE_IO);

    // TODO: IO mode later
    if (exo->uart.uarts[port]->io_mode)
    {
#if (UART_IO_MODE_SUPPORT)
        ok = lpc_uart_open_io(exo, port);
#else
        ok = false;
#endif //UART_IO_MODE_SUPPORT
    }
    else
        ok = nrf_uart_open_stream(exo->uart.uarts[port], port, mode);
    if (!ok)
    {
        nrf_uart_destroy(exo, port);
        return;
    }

    //power up

    // set TX_PIN
    UART_REGS[port]->PSELTXD = P9; // TODO: temporary UART pin define
//    NRF_UART0->PSELRXD = UART_RX_PIN;
    // diconnected pin
    UART_REGS[port]->PSELRTS       = 0xFFFFFFFF;
    UART_REGS[port]->PSELCTS       = 0xFFFFFFFF;

    UART_REGS[port]->EVENTS_CTS = 0;
    UART_REGS[port]->EVENTS_NCTS = 0;

    UART_REGS[port]->EVENTS_RXDRDY = 0;

    // Enable UART RX interrupt only
    //NRF_UART0->INTENSET = (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);

    // disable flow contorl
    UART_REGS[port]->CONFIG   = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
    UART_REGS[port]->INTENCLR |= UART_INTENCLR_CTS_Msk | UART_INTENCLR_RXDRDY_Msk;

    UART_REGS[port]->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    // start transmission
    UART_REGS[port]->TASKS_STARTTX = 1;
    // start reception
//    UART_REGS[port]->TASKS_STARTRX = 1;

    NRF_UART0->INTENCLR = 0xffffffffUL;
    NRF_UART0->INTENSET = (UART_INTENSET_ERROR_Set << UART_INTENSET_ERROR_Pos);

    kirq_register(KERNEL_HANDLE, UART_VECTORS[port], nrf_uart_on_isr, (void*)exo);

    NVIC_ClearPendingIRQ(UART_VECTORS[port]);
    NVIC_EnableIRQ(UART_VECTORS[port]);
#if defined(UART_IRQ_PRIORITY)
    NVIC_SetPriority(UART_VECTORS[port], UART_IRQ_PRIORITY);
#else
    NVIC_SetPriority(UART_VECTORS[port], 13);
#endif // UART_IRQ_PRIORITY
}

static inline void nrf_uart_close(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    //disable interrupts
    NVIC_DisableIRQ(UART_VECTORS[port]);
    kirq_unregister(KERNEL_HANDLE, UART_VECTORS[port]);

    // uart destroy
    nrf_uart_flush_internal(exo, port);
    nrf_uart_destroy(exo, port);
}

static inline void nrf_uart_flush(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    nrf_uart_flush_internal(exo, port);
}

static inline HANDLE nrf_uart_get_tx_stream(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return INVALID_HANDLE;
    }

    return exo->uart.uarts[port]->s.tx_stream;
}

static inline HANDLE nrf_uart_get_rx_stream(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return INVALID_HANDLE;
    }
    return exo->uart.uarts[port]->s.rx_stream;
}

static inline uint16_t nrf_uart_get_last_error(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return ERROR_OK;
    }
    return exo->uart.uarts[port]->error;
}

static inline void nrf_uart_clear_error(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    exo->uart.uarts[port]->error = ERROR_OK;
}

static inline void nrf_uart_setup_printk(EXO* exo, UART_PORT port)
{
    //setup kernel printk dbg
    kernel_setup_dbg(uart_write_kernel, (void*)port);
}

void nrf_uart_write(EXO* exo, UART_PORT port, unsigned int total)
{
    UART* uart = exo->uart.uarts[port];
    uart->s.tx_total = uart->s.tx_size = kstream_read_no_block(uart->s.tx_handle, uart->s.tx_buf, UART_BUF_SIZE);
    // enable TRANSMIT and TXEMPTY IRQ
    UART_REGS[port]->INTENSET |= UART_INTENSET_TXDRDY_Msk;
    // send first byte
    UART_REGS[port]->TXD = uart->s.tx_buf[uart->s.tx_total - uart->s.tx_size--];

}

void nrf_uart_request(EXO* exo, IPC* ipc)
{
    UART_PORT port = (UART_PORT)ipc->param1;
    if (port >= UARTS_COUNT)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        nrf_uart_open(exo, port, ipc->param2);
        break;
    case IPC_CLOSE:
        nrf_uart_close(exo, port);
        break;
    case IPC_UART_SET_BAUDRATE:
        nrf_uart_set_baudrate(exo, port, ipc);
        break;
    case IPC_FLUSH:
        nrf_uart_flush(exo, port);
        break;
    case IPC_GET_TX_STREAM:
        ipc->param2 = nrf_uart_get_tx_stream(exo, port);
        break;
    case IPC_GET_RX_STREAM:
        ipc->param2 = nrf_uart_get_rx_stream(exo, port);
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param2 = nrf_uart_get_last_error(exo, port);
        break;
    case IPC_UART_CLEAR_ERROR:
        nrf_uart_clear_error(exo, port);
        break;
    case IPC_UART_SETUP_PRINTK:
        nrf_uart_setup_printk(exo, port);
        break;
    case IPC_STREAM_WRITE:
        nrf_uart_write(exo, port, ipc->param3);
        //message from kernel, no response
        break;
#if (UART_IO_MODE_SUPPORT)
    case IPC_READ:
        nrf_uart_io_read(exo, port, ipc);
        break;
    case IPC_WRITE:
        nrf_uart_io_write(exo, port, ipc);
        break;
    case IPC_TIMEOUT:
        nrf_uart_io_read_timeout(exo, port);
        break;
    case IPC_UART_SET_COMM_TIMEOUTS:
        nrf_uart_set_timeouts(exo, port, ipc->param2, ipc->param3);
        break;
#endif //UART_IO_MODE_SUPPORT
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void nrf_uart_init(EXO* exo)
{
    int i;
    for (i = 0; i < UARTS_COUNT; ++i)
        exo->uart.uarts[i] = NULL;
}
