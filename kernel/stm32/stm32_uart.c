/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "error.h"
#include "../kstdlib.h"
#include "../kstream.h"
#include "../kirq.h"
#include <string.h>
#include "stm32_exo_private.h"

typedef enum {
    IPC_UART_ISR_RX = IPC_UART_MAX
} STM32_UART_IPCS;

typedef USART_TypeDef* USART_TypeDef_P;

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2};

#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3};

#elif defined(STM32F1)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19, 20};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5};

#elif defined(STM32F2) || defined(STM32F40_41xxx)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6};

#elif defined(STM32F4)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71, 82, 83};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5, 30, 31};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6, UART7, UART8};
#elif defined(STM32L0)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {27, 28};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2};
#elif defined(STM32L1)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3};
#elif defined(STM32F0)
#if ((UARTS_COUNT)== 1)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {27};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1};
#elif ((UARTS_COUNT)== 2)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {27, 28};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2};
#elif ((UARTS_COUNT)== 4)
static const unsigned int UART_VECTORS[3] =                 {27, 28, 29};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, USART4};
#elif ((UARTS_COUNT)== 6)
static const unsigned int UART_VECTORS[3] =                 {27, 28, 29};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19, 20, 5};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, USART4, USART5, USART6};
#else
static const unsigned int UART_VECTORS[3] =                 {27, 28, 29};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19, 20, 5, 6, 7};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, USART4, USART5, USART6, USART7, USART8};
#endif //UARTS_COUNT
#endif

#if defined(STM32L0) || defined(STM32F0)
#define USART_SR_TXE        USART_ISR_TXE
#define USART_SR_TC         USART_ISR_TC
#define USART_SR_PE         USART_ISR_PE
#define USART_SR_FE         USART_ISR_FE
#define USART_SR_NE         USART_ISR_NE
#define USART_SR_ORE        USART_ISR_ORE
#define USART_SR_RXNE       USART_ISR_RXNE
#define SR(port)            (UART_REGS[(port)]->ISR)
#define TXC(port, c)        (UART_REGS[(port)]->TDR = (c))
#else
#define SR(port)            (UART_REGS[(port)]->SR)
#define TXC(port, c)        (UART_REGS[(port)]->DR = (c))
#endif


static inline bool stm32_uart_on_tx_isr(EXO* exo, UART_PORT port)
{
    UART* uart = exo->uart.uarts[port];
#if (UART_IO_MODE_SUPPORT)
    if (uart->io_mode)
    {
        if (uart->i.tx_io)
        {
            TXC(port, ((uint8_t*)io_data(uart->i.tx_io))[uart->i.tx_processed++]);
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
                stream_ilisten(uart->s.tx_stream, port, HAL_UART);
                return false;
            }
        }
        TXC(port, uart->s.tx_buf[uart->s.tx_total - uart->s.tx_size--]);
        return  true;
    }
    return false;
}

static inline void stm32_uart_on_rx_isr(EXO* exo, UART_PORT port, uint8_t c)
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
                uart->i.rx_io = NULL;
            }
            else
                timer_istart_ms(uart->i.rx_timer, uart->i.rx_interleaved_timeout);
        }
    }
    else
#endif //UART_IO_MODE_SUPPORT
        ipc_ipost_inline(process_iget_current(), HAL_CMD(HAL_UART, IPC_UART_ISR_RX), port, 0, c);
}

void stm32_uart_on_isr(int vector, void* param)
{
    //find port by vector
    UART_PORT port;
    uint8_t c;
#if defined(STM32L0) || defined(STM32F0)
    unsigned int sr = 0;
#else
    uint16_t sr;
#endif
    EXO* exo = (EXO*)param;

    for (port = UART_1; port < UART_MAX && UART_VECTORS[port] != vector; ++port) {}

#if defined(STM32F0) && (UARTS_COUNT > 3)
    if (port == UART_3)
    {
        for (port = UART_3; port < UARTS_COUNT; ++port)
        {
            if (exo->uart.uarts[port] == NULL)
                continue;
            sr = UART_REGS[port]->ISR;
            if (((UART_REGS[port]->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) ||
                ((UART_REGS[port]->CR1 & USART_CR1_TCIE) && (sr & USART_SR_TC)) ||
                (sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)) ||
                (sr & USART_SR_RXNE))
                break;
        }
        if (port == UARTS_COUNT)
            return;
    }
    else
#endif
        sr = SR(port);

    //decode error, if any
    if (sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE))
    {
        if (sr & USART_SR_ORE)
            exo->uart.uarts[port]->error = ERROR_OVERFLOW;
        else
        {
#if defined(STM32L0) || defined(STM32F0)
            __REG_RC32(UART_REGS[port]->RDR);
#else
            __REG_RC32(UART_REGS[port]->DR);
#endif
            if (sr & USART_SR_FE)
                exo->uart.uarts[port]->error = ERROR_INVALID_FRAME;
            else if (sr & USART_SR_PE)
                exo->uart.uarts[port]->error = ERROR_INVALID_PARITY;
            else if  (sr & USART_SR_NE)
                exo->uart.uarts[port]->error = ERROR_LINE_NOISE;
        }
    }

    //transmit more
    if (UART_REGS[port]->CR1 & USART_CR1_TXEIE)
    {
        if (sr & USART_SR_TXE)
        {
            if (!stm32_uart_on_tx_isr(exo, port))
            {
                UART_REGS[port]->CR1 &= ~USART_CR1_TXEIE;
                UART_REGS[port]->CR1 |= USART_CR1_TCIE;
            }
        }
    }
    //transmission completed and no more data. Disable transmitter
    if ((UART_REGS[port]->CR1 & USART_CR1_TCIE) && (SR(port) & USART_SR_TC))
        UART_REGS[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);

    //receive data
    if (SR(port) & USART_SR_RXNE)
    {
#if defined(STM32L0) || defined(STM32F0)
        c = UART_REGS[port]->RDR;
#else
        c = UART_REGS[port]->DR;
#endif
         stm32_uart_on_rx_isr(exo, port, c);
    }
}

void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
    NVIC_DisableIRQ(UART_VECTORS[(UART_PORT)param]);
    int i;
    UART_REGS[(UART_PORT)param]->CR1 |= USART_CR1_TE;
    for(i = 0; i < size; ++i)
    {
#if defined(STM32L0) || defined(STM32F0)
        while ((UART_REGS[(UART_PORT)param]->ISR & USART_ISR_TXE) == 0) {}
        UART_REGS[(UART_PORT)param]->TDR = buf[i];
#else
        while ((UART_REGS[(UART_PORT)param]->SR & USART_SR_TXE) == 0) {}
        UART_REGS[(UART_PORT)param]->DR = buf[i];
#endif
    }
    UART_REGS[(UART_PORT)param]->CR1 |= USART_CR1_TCIE;
    //transmitter will be disabled in next IRQ TC
    NVIC_EnableIRQ(UART_VECTORS[(UART_PORT)param]);
}

static inline void stm32_uart_set_baudrate(EXO* exo, UART_PORT port, IPC* ipc)
{
    BAUD baudrate;
    unsigned int clock, stop;
    if (exo->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    uart_decode_baudrate(ipc, &baudrate);

    UART_REGS[port]->CR1 &= ~USART_CR1_UE;

    if (baudrate.data_bits == 8 && baudrate.parity != 'N')
#if defined(STM32L0)
        UART_REGS[port]->CR1 |= USART_CR1_M_0;
#elif defined(STM32F0)
        UART_REGS[port]->CR1 |= USART_CR1_M0;
#else
        UART_REGS[port]->CR1 |= USART_CR1_M;
#endif
    else
#if defined(STM32L0)
        UART_REGS[port]->CR1 &= ~USART_CR1_M_0;
#elif defined(STM32F0)
        UART_REGS[port]->CR1 &= ~USART_CR1_M0;
#else
        UART_REGS[port]->CR1 &= ~USART_CR1_M;
#endif

    if (baudrate.parity != 'N')
    {
        UART_REGS[port]->CR1 |= USART_CR1_PCE;
        if (baudrate.parity == 'O')
            UART_REGS[port]->CR1 |= USART_CR1_PS;
        else
            UART_REGS[port]->CR1 &= ~USART_CR1_PS;
    }
    else
        UART_REGS[port]->CR1 &= ~USART_CR1_PCE;

    switch (baudrate.stop_bits)
    {
    case 2:
        stop = 2;
        break;
    //0.5
    case 5:
        stop = 1;
        break;
    //1.5
    case 15:
        stop = 3;
        break;
    default:
        stop = 0;
    }

    UART_REGS[port]->CR2 &= ~(3 << 12);
    UART_REGS[port]->CR2 |= (stop << 12);

    if (port == UART_1 || port >= UART_6)
        clock = stm32_power_get_clock_inside(exo, STM32_CLOCK_APB2);
    else
        clock = stm32_power_get_clock_inside(exo, STM32_CLOCK_APB1);
    unsigned int mantissa, fraction;
    mantissa = (25 * clock) / (4 * (baudrate.baud));
    fraction = ((mantissa % 100) * 8 + 25)  / 50;
    mantissa = mantissa / 100;
    UART_REGS[port]->BRR = (mantissa << 4) | fraction;

    UART_REGS[port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
    UART_REGS[port]->CR3 |= USART_CR3_EIE;
}

static void stm32_uart_flush_internal(EXO* exo, UART_PORT port)
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
            io_complete_ex_exo(exo->uart.uarts[port]->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, rx_io, ERROR_IO_CANCELLED);
        if (tx_io)
            io_complete_ex_exo(exo->uart.uarts[port]->i.tx_process, HAL_IO_CMD(HAL_UART, IPC_WRITE), port, tx_io, ERROR_IO_CANCELLED);
        timer_stop(exo->uart.uarts[port]->i.rx_timer, port, HAL_UART);
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

static void stm32_uart_destroy(EXO* exo, UART_PORT port)
{
#if (UART_IO_MODE_SUPPORT)
    if (exo->uart.uarts[port]->io_mode)
    {
        timer_destroy(exo->uart.uarts[port]->i.rx_timer);
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
    if (port == UART_1 || port >= UART_6)
        RCC->APB2ENR &= ~(1 << UART_POWER_PINS[port]);
    else
        RCC->APB1ENR &= ~(1 << UART_POWER_PINS[port]);
}

static inline bool stm32_uart_open_stream(UART* uart, UART_PORT port, unsigned int mode)
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
        UART_REGS[port]->CR1 |= USART_CR1_RE | USART_CR1_RXNEIE;
    }
    return true;
}

#if (UART_IO_MODE_SUPPORT)
static inline bool stm32_uart_open_io(UART* uart, UART_PORT port)
{
    uart->i.tx_io = uart->i.rx_io = NULL;
    uart->i.rx_timer = timer_create(port, HAL_UART);
    uart->i.rx_char_timeout = UART_CHAR_TIMEOUT_MS;
    uart->i.rx_interleaved_timeout = UART_INTERLEAVED_TIMEOUT_MS;
    if (uart->i.rx_timer != INVALID_HANDLE)
    {
        //enable receiver
        UART_REGS[port]->CR1 |= USART_CR1_RXNEIE | USART_CR1_RE;
        return true;
    }
    return false;
}
#endif //UART_IO_MODE_SUPPORT

static inline void stm32_uart_open(EXO* exo, UART_PORT port, unsigned int mode)
{
    bool ok;
    if (exo->uart.uarts[port] != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    exo->uart.uarts[port] = kmalloc(sizeof(UART));
    if (exo->uart.uarts[port] == NULL)
        return;
    exo->uart.uarts[port]->error = ERROR_OK;
#if (UART_IO_MODE_SUPPORT)
    exo->uart.uarts[port]->io_mode = ((mode & UART_MODE) == UART_MODE_IO);
#endif //UART_IO_MODE_SUPPORT

    //power up (required prior to reg work)
    if (port == UART_1 || port >= UART_6)
        RCC->APB2ENR |= 1 << UART_POWER_PINS[port];
    else
        RCC->APB1ENR |= 1 << UART_POWER_PINS[port];
    UART_REGS[port]->CR1 = 0;

#if (UART_IO_MODE_SUPPORT)
    if (exo->uart.uarts[port]->io_mode)
        ok = stm32_uart_open_io(exo->uart.uarts[port], port);
    else
#endif //UART_IO_MODE_SUPPORT
        ok = stm32_uart_open_stream(exo->uart.uarts[port], port, mode);
    if (!ok)
    {
        stm32_uart_destroy(exo, port);
        return;
    }

    //enable core
    UART_REGS[port]->CR1 |= USART_CR1_UE;

    //enable interrupts
#if defined(STM32F0) && (UARTS_COUNT > 3)
    if (port >= UART_3)
    {
        if (exo->uart.isr3_cnt++ == 0)
        {
            kirq_register(KERNEL_HANDLE, UART_VECTORS[UART_3], stm32_uart_on_isr, (void*)exo);
            NVIC_EnableIRQ(UART_VECTORS[UART_3]);
            NVIC_SetPriority(UART_VECTORS[UART_3], 13);
        }
    }
    else
#endif
    {
        kirq_register(KERNEL_HANDLE, UART_VECTORS[port], stm32_uart_on_isr, (void*)exo);
        NVIC_EnableIRQ(UART_VECTORS[port]);
        NVIC_SetPriority(UART_VECTORS[port], 13);
    }
}

static inline void stm32_uart_close(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //disable interrupts
#if defined(STM32F0) && (UARTS_COUNT > 3)
    if (port >= UART_3)
    {
        if (--exo->uart.isr3_cnt == 0)
        {
            NVIC_DisableIRQ(UART_VECTORS[UART_3]);
            kirq_unregister(KERNEL_HANDLE, UART_VECTORS[UART_3]);
        }
    }
    else
#endif
    {
        NVIC_DisableIRQ(UART_VECTORS[port]);
        kirq_unregister(KERNEL_HANDLE, UART_VECTORS[port]);
    }

    //disable core
    UART_REGS[port]->CR1 &= ~USART_CR1_UE;
    stm32_uart_flush_internal(exo, port);
    stm32_uart_destroy(exo, port);
}

static inline void stm32_uart_flush(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    stm32_uart_flush_internal(exo, port);
}

static inline HANDLE stm32_uart_get_tx_stream(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return INVALID_HANDLE;
    }
    return exo->uart.uarts[port]->s.tx_stream;
}

static inline HANDLE stm32_uart_get_rx_stream(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return INVALID_HANDLE;
    }
    return exo->uart.uarts[port]->s.rx_stream;
}

static inline uint16_t stm32_uart_get_last_error(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return ERROR_OK;
    }
    return exo->uart.uarts[port]->error;
}

static inline void stm32_uart_clear_error(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    exo->uart.uarts[port]->error = ERROR_OK;
}

static inline void stm32_uart_setup_printk(EXO* exo, UART_PORT port)
{
    //setup kernel printk dbg
    kernel_setup_dbg(uart_write_kernel, (void*)port);
//    setup_dbg(uart_write_kernel, (void*)port);
}

void stm32_uart_write(EXO* exo, UART_PORT port, unsigned int total)
{
    UART* uart = exo->uart.uarts[port];
    uart->s.tx_total = uart->s.tx_size = kstream_read_no_block(uart->s.tx_handle, uart->s.tx_buf, UART_BUF_SIZE);
    //start transaction
    UART_REGS[port]->CR1 |= USART_CR1_TE | USART_CR1_TXEIE;
}

static inline void stm32_uart_read(EXO* exo, UART_PORT port, char c)
{
    //caching calls to svc
    if (exo->uart.uarts[port]->s.rx_free == 0)
        (exo->uart.uarts[port]->s.rx_free = kstream_get_free(exo->uart.uarts[port]->s.rx_stream));
    //if stream is full, char will be discarded
    if (exo->uart.uarts[port]->s.rx_free)
    {
        kstream_write(KERNEL_HANDLE, exo->uart.uarts[port]->s.rx_handle, &c, 1);
        exo->uart.uarts[port]->s.rx_free--;
    }
}

#if (UART_IO_MODE_SUPPORT)
static inline void stm32_uart_io_read(EXO* exo, UART_PORT port, IPC* ipc)
{
    UART* uart = exo->uart.uarts[port];
    IO* io;
    if (uart == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (!uart->io_mode)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if (uart->i.rx_io)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    io = (IO*)ipc->param2;
    uart->i.rx_process = ipc->process;
    uart->i.rx_max = ipc->param3;
    io->data_size = 0;
    timer_start_ms(uart->i.rx_timer, uart->i.rx_char_timeout);
    uart->i.rx_io = io;
    error(ERROR_SYNC);
}

static inline void stm32_uart_io_write(EXO* exo, UART_PORT port, IPC* ipc)
{
    UART* uart = exo->uart.uarts[port];
    IO* io;
    if (uart == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (!uart->io_mode)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if (uart->i.tx_io)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    io = (IO*)ipc->param2;
    uart->i.tx_process = ipc->process;
    uart->i.tx_processed = 0;
    uart->i.tx_io = io;
    //start
    UART_REGS[port]->CR1 |= USART_CR1_TE | USART_CR1_TXEIE;
    error(ERROR_SYNC);
}

static inline void stm32_uart_io_read_timeout(EXO* exo, UART_PORT port)
{
    IO* io = NULL;
    UART* uart = exo->uart.uarts[port];
    timer_stop(uart->i.rx_timer, port, HAL_UART);
    __disable_irq();
    if (uart->i.rx_io)
    {
        io = uart->i.rx_io;
        uart->i.rx_io = NULL;
    }
    __enable_irq();
    if (io)
    {
        if (io->data_size)
            io_complete(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, io);
        else
            //no data? timeout
            io_complete_ex_exo(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, io, ERROR_TIMEOUT);
    }
}
#endif //UART_IO_MODE_SUPPORT

void stm32_uart_request(EXO* exo, IPC* ipc)
{
    UART_PORT port = (UART_PORT)ipc->param1;
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_uart_open(exo, port, ipc->param2);
        break;
    case IPC_CLOSE:
        stm32_uart_close(exo, port);
        break;
    case IPC_UART_SET_BAUDRATE:
        stm32_uart_set_baudrate(exo, port, ipc);
        break;
    case IPC_FLUSH:
        stm32_uart_flush(exo, port);
        break;
    case IPC_GET_TX_STREAM:
        ipc->param2 = stm32_uart_get_tx_stream(exo, port);
        break;
    case IPC_GET_RX_STREAM:
        ipc->param2 = stm32_uart_get_rx_stream(exo, port);
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param2 = stm32_uart_get_last_error(exo, port);
        break;
    case IPC_UART_CLEAR_ERROR:
        stm32_uart_clear_error(exo, port);
        break;
    case IPC_UART_SETUP_PRINTK:
        stm32_uart_setup_printk(exo, port);
        break;
    case IPC_STREAM_WRITE:
        stm32_uart_write(exo, port, ipc->param3);
        //message from kernel, no response
        break;
    case IPC_UART_ISR_RX:
        stm32_uart_read(exo, port, ipc->param3);
        //message from ISR, no response
        break;
#if (UART_IO_MODE_SUPPORT)
    case IPC_READ:
        stm32_uart_io_read(exo, port, ipc);
        break;
    case IPC_WRITE:
        stm32_uart_io_write(exo, port, ipc);
        break;
    case IPC_TIMEOUT:
        stm32_uart_io_read_timeout(exo, port);
        break;
#endif //UART_IO_MODE_SUPPORT
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_uart_init(EXO* exo)
{
    int i;
    for (i = 0; i < UARTS_COUNT; ++i)
        exo->uart.uarts[i] = NULL;
#if defined(STM32F0) && (UARTS_COUNT > 3)
    exo->uart.isr3_cnt = 0;
#endif
}

