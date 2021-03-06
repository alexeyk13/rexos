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

static const unsigned int UART_VECTORS[UARTS_COUNT] = {UART0_IRQn};
static const NRF_UART_Type_P UART_REGS[UARTS_COUNT] = {NRF_UART0};

#define NRF_SPI_IRQ                                   SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn

#define TXC(port, c)                                  (UART_REGS[(port)]->TXD = (c))


static inline bool nrf_uart_on_tx_isr(EXO* exo, UART_PORT port)
{
    UART* uart = exo->uart.uarts[port];
#if (UART_IO_MODE_SUPPORT)
    if (uart->io_mode)
    {
        if (uart->i.tx_io)
        {
            if (uart->i.tx_processed < uart->i.tx_io->data_size)
            {
                TXC(port, ((uint8_t*)io_data(uart->i.tx_io))[uart->i.tx_processed++]);
                return true;
            }
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
    UART_PORT port = UART_0;
    EXO* exo = (EXO*)param;
    uint32_t sr = UART_REGS[port]->ERRORSRC;

    if (UART_REGS[port]->EVENTS_TXDRDY != 0)
    {
        UART_REGS[port]->EVENTS_TXDRDY = 0;
        nrf_uart_on_tx_isr(exo, port);
    }

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

    if (port == UART_SPI_0)
        return;

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

    if(baudrate.parity != 'N')
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
}
//---------------------------- IO_MODE  ----------
#if (UART_IO_MODE_SUPPORT)
static inline bool nrf_uart_open_io(UART* uart, UART_PORT port)
{
    uart->i.tx_io = uart->i.rx_io = NULL;
#if (UART_DOUBLE_BUFFERING)
    uart->i.rx2_io = NULL;
#endif // UART_DOUBLE_BUFFERING

    uart->i.rx_timer = ksystime_soft_timer_create(KERNEL_HANDLE, port, HAL_UART);

    uart->i.rx_char_timeout = UART_CHAR_TIMEOUT_US;
    uart->i.rx_interleaved_timeout = UART_INTERLEAVED_TIMEOUT_US;
    if (uart->i.rx_timer != INVALID_HANDLE)
    {
        //enable receiver
        return true;
    }
    return false;
}


static inline void nrf_uart_io_read(EXO* exo, UART_PORT port, IPC* ipc)
{
    UART* uart = exo->uart.uarts[port];
    IO* io;
    if (uart == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if (!uart->io_mode)
    {
        kerror(ERROR_INVALID_STATE);
        return;
    }
#if (UART_DOUBLE_BUFFERING)
    if (uart->i.rx2_io)
#else
    if (uart->i.rx_io)
#endif // UART_DOUBLE_BUFFERING
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    io = (IO*)ipc->param2;
    uart->i.rx_process = ipc->process;
    io->data_size = 0;
#if (UART_DOUBLE_BUFFERING)
    __disable_irq();
    if (uart->i.rx_io)
    {
        uart->i.rx2_max = ipc->param3;
        uart->i.rx2_io = io;
        __enable_irq();
    }else
#endif // UART_DOUBLE_BUFFERING
    {
        __enable_irq();
        uart->i.rx_max = ipc->param3;
        ksystime_soft_timer_start_us(uart->i.rx_timer, uart->i.rx_char_timeout);
        uart->i.rx_io = io;
    }
    kerror(ERROR_SYNC);
}

static inline void nrf_uart_io_write(EXO* exo, UART_PORT port, IPC* ipc)
{
    UART* uart = exo->uart.uarts[port];
    IO* io;
    if (uart == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if (!uart->io_mode)
    {
        kerror(ERROR_INVALID_STATE);
        return;
    }
    if (uart->i.tx_io)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    io = (IO*)ipc->param2;
    uart->i.tx_process = ipc->process;
    uart->i.tx_processed = 0;
    uart->i.tx_io = io;

    TXC(port, ((uint8_t*)io_data(uart->i.tx_io))[uart->i.tx_processed++]);
    kerror(ERROR_SYNC);
}
static inline void nrf_uart_set_timeouts(EXO* exo, UART_PORT port, uint32_t char_timeout_us, uint32_t interleaved_timeout_us)
{
    UART* uart = exo->uart.uarts[port];
    if (uart == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if (!uart->io_mode)
    {
        kerror(ERROR_INVALID_STATE);
        return;
    }
    uart->i.rx_char_timeout = char_timeout_us;
    uart->i.rx_interleaved_timeout = interleaved_timeout_us;
}

static inline void nrf_uart_io_read_timeout(EXO* exo, UART_PORT port)
{
    IO* io = NULL;
    UART* uart = exo->uart.uarts[port];
    ksystime_soft_timer_stop(uart->i.rx_timer);
    __disable_irq();
    if (uart->i.rx_io)
    {
        io = uart->i.rx_io;
        uart->i.rx_io = NULL;
    }
#if (UART_DOUBLE_BUFFERING)
    if (uart->i.rx2_io)
    {
        uart->i.rx_io = uart->i.rx2_io;
        uart->i.rx_max = uart->i.rx2_max;
        uart->i.rx2_io = NULL;
        __enable_irq();
        timer_istart_us(uart->i.rx_timer, uart->i.rx_char_timeout);
    }
#endif // UART_DOUBLE_BUFFERING
    __enable_irq();
    if (io)
    {
        if (io->data_size)
            kexo_io(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, io);
        else
            kexo_io_ex(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, io, ERROR_TIMEOUT);
    }
}

#endif //UART_IO_MODE_SUPPORT
//---------------------------- UART_SPI_0  ----------
static inline void  nrf_uart_spi_init_send(uint8_t* buf, char* src, uint32_t len)
{
    if(len == 0 || len > UART_BUF_SIZE)
        return;
    NRF_SPIM0->TXD.MAXCNT = 2*len+1;
    NRF_SPIM0->TXD.PTR = (uint32_t)buf;

    do{
        *buf++ = 0x7f;
        *buf++ = *src++;
    }while(--len);
    *buf++ = 0xff;
    NRF_SPIM0->TASKS_START = 1;
}


#define PRINTK_BUF_SIZE        15
static inline void uart_spi_write_kernel(const char *const buf, unsigned int size, void* param)
{
    uint8_t prom[1 + 2*PRINTK_BUF_SIZE];
    NVIC_DisableIRQ(NRF_SPI_IRQ);
    uint32_t chunk;
    char* buf_ptr = (char*)buf;

    if(NRF_SPIM0->EVENTS_STARTED)
            while(NRF_SPIM0->EVENTS_END == 0);

    while(size)
    {
        chunk = UART_BUF_SIZE;
        if(chunk > size)
            chunk = size;
        NRF_SPIM0->EVENTS_END = 0;
        nrf_uart_spi_init_send(prom, buf_ptr, chunk);
        while(NRF_SPIM0->EVENTS_END == 0);
//        NRF_SPIM0->EVENTS_STARTED = 0;
        size -= chunk;
        buf_ptr = &buf_ptr[chunk];
    }
    NVIC_EnableIRQ(NRF_SPI_IRQ);
}

static inline void nrf_uart_spi_on_isr(int vector, void* param)
{
    UART_PORT port = UART_SPI_0;
    EXO* exo = (EXO*)param;
    UART* uart = exo->uart.uarts[port];
    NRF_SPIM0->EVENTS_END = 0;
    NRF_SPIM0->EVENTS_STARTED = 0;
    uart->s.tx_size = stream_iread_no_block(uart->s.tx_handle, uart->s.tx_buf, UART_BUF_SIZE);
    if (uart->s.tx_size == 0)
        stream_ilisten(uart->s.tx_stream, port, HAL_UART);
    else
        nrf_uart_spi_init_send(exo->uart.spi_buf, uart->s.tx_buf, uart->s.tx_size);
}

static inline void nrf_uart_spi_open(EXO* exo)
{
    exo->uart.spi_buf = malloc(UART_SPI_BUF_SIZE);
    NRF_SPIM0->CONFIG = 1;
    NRF_SPIM0->FREQUENCY = 0x01D80000; // 115200
    NRF_SPIM0->ENABLE = 7;
    NRF_SPIM0->TXD.PTR = (uint32_t)exo->uart.spi_buf;

    NRF_SPIM0->INTENSET = SPIM_INTENSET_END_Msk;
    kirq_register(KERNEL_HANDLE, NRF_SPI_IRQ, nrf_uart_spi_on_isr, (void*)exo);
    NVIC_ClearPendingIRQ(NRF_SPI_IRQ);
    NVIC_EnableIRQ(NRF_SPI_IRQ);
    NVIC_SetPriority(NRF_SPI_IRQ, 2);//13
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

    if (port == UART_SPI_0 && exo->uart.uarts[port]->io_mode)
    {
        kerror(ERROR_NOT_SUPPORTED);
        return;
    }

    if (exo->uart.uarts[port]->io_mode)
    {
#if (UART_IO_MODE_SUPPORT)
        ok = nrf_uart_open_io(exo->uart.uarts[port], port);
#else
        ok = false;
#endif //UART_IO_MODE_SUPPORT
    }else
        ok = nrf_uart_open_stream(exo->uart.uarts[port], port, mode);

    if (!ok)
    {
        nrf_uart_destroy(exo, port);
        return;
    }
    if (port == UART_SPI_0)
    {
        nrf_uart_spi_open(exo);
        return;
    }

    NRF_UART0->INTENCLR = 0xffffffffUL;
    NRF_UART0->INTENSET = (UART_INTENSET_ERROR_Set << UART_INTENSET_ERROR_Pos);

    UART_REGS[port]->TASKS_STARTTX = 1;
    UART_REGS[port]->INTENSET |= UART_INTENSET_TXDRDY_Msk;

    UART_REGS[port]->CONFIG   = 0; // disable flow control and parity
    UART_REGS[port]->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    // start reception
    UART_REGS[port]->EVENTS_RXDRDY = 0;
    UART_REGS[port]->INTENSET |= UART_INTENSET_RXDRDY_Msk;
    UART_REGS[port]->TASKS_STARTRX = 1;


    kirq_register(KERNEL_HANDLE, UART_VECTORS[port], nrf_uart_on_isr, (void*)exo);
    NVIC_ClearPendingIRQ(UART_VECTORS[port]);
    NVIC_EnableIRQ(UART_VECTORS[port]);
    NVIC_SetPriority(UART_VECTORS[port], 2);// 12
}

static inline void nrf_uart_close(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if (port == UART_SPI_0)
    {
        NRF_SPIM0->INTENCLR = SPIM_INTENSET_END_Msk;
        NRF_SPIM0->ENABLE = 0;

        NVIC_DisableIRQ(NRF_SPI_IRQ);
        kirq_unregister(KERNEL_HANDLE, NRF_SPI_IRQ);
    }else{
        NVIC_DisableIRQ(UART_VECTORS[port]);
        kirq_unregister(KERNEL_HANDLE, UART_VECTORS[port]);
    }
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
    if(port == UART_SPI_0)
        kernel_setup_dbg(uart_spi_write_kernel, (void*)port);
    else
        kernel_setup_dbg(uart_write_kernel, (void*)port);
}

static void nrf_uart_write(EXO* exo, UART_PORT port, unsigned int total)
{
    UART* uart = exo->uart.uarts[port];
    uart->s.tx_total = uart->s.tx_size = kstream_read_no_block(uart->s.tx_handle, uart->s.tx_buf, UART_BUF_SIZE);
    if (port == UART_SPI_0)
    {
        nrf_uart_spi_init_send(exo->uart.spi_buf,uart->s.tx_buf, uart->s.tx_total);
    }else{
        TXC(port, uart->s.tx_buf[uart->s.tx_total - uart->s.tx_size--]);
    }
}


void nrf_uart_request(EXO* exo, IPC* ipc)
{
    UART_PORT port = (UART_PORT)ipc->param1;
    if (port >= (UARTS_COUNT+1))
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
    memset(&exo->uart, 0, sizeof(UART_DRV));
}
