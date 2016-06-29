/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "error.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stream.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include <string.h>
#include "stm32_core_private.h"

#define get_clock               stm32_power_get_clock_inside

typedef enum {
    IPC_UART_ISR_TX = IPC_UART_MAX,
    IPC_UART_ISR_RX
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
#endif

void stm32_uart_on_isr(int vector, void* param)
{
    IPC ipc;
    //find port by vector
    UART_PORT port;
#if defined(STM32L0) || defined(STM32F0)
    unsigned int sr = 0;
#else
    uint16_t sr;
#endif
    CORE* core = (CORE*)param;

    for (port = UART_1; port < UART_MAX && UART_VECTORS[port] != vector; ++port) {}

#if defined(STM32F0) && (UARTS_COUNT > 3)
    if (port == UART_3)
    {
        for (port = UART_3; port < UARTS_COUNT; ++port)
        {
            if (core->uart.uarts[port] == NULL)
                continue;
            sr = UART_REGS[port]->ISR;
            if (((UART_REGS[port]->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE) && core->uart.uarts[port]->tx_chunk_size) ||
                ((UART_REGS[port]->CR1 & USART_CR1_TCIE) && (sr & USART_SR_TC)) ||
                (sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)) ||
                (sr & USART_SR_RXNE))
                break;
        }
    }
    else
#endif
#if defined(STM32L0) || defined(STM32F0)
        sr = UART_REGS[port]->ISR;
#else
    sr = UART_REGS[port]->SR;
#endif

    //transmit more
    if ((UART_REGS[port]->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE) && core->uart.uarts[port]->tx_chunk_size)
    {
#if defined(STM32L0) || defined(STM32F0)
        UART_REGS[port]->TDR = core->uart.uarts[port]->tx_buf[core->uart.uarts[port]->tx_chunk_pos++];
#else
        UART_REGS[port]->DR = core->uart.uarts[port]->tx_buf[core->uart.uarts[port]->tx_chunk_pos++];
#endif
        //no more
        if (core->uart.uarts[port]->tx_chunk_pos >= core->uart.uarts[port]->tx_chunk_size)
        {
            core->uart.uarts[port]->tx_chunk_pos = core->uart.uarts[port]->tx_chunk_size = 0;
            ipc.process = process_iget_current();
            ipc.cmd = HAL_CMD(HAL_UART, IPC_UART_ISR_TX);
            ipc.param1 = port;
            ipc.param3 = 0;
            ipc_ipost(&ipc);
            UART_REGS[port]->CR1 &= ~USART_CR1_TXEIE;
            UART_REGS[port]->CR1 |= USART_CR1_TCIE;

        }
    }
    //transmission completed and no more data. Disable transmitter
    else if ((UART_REGS[port]->CR1 & USART_CR1_TCIE) && (sr & USART_SR_TC))
        UART_REGS[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
    //decode error, if any
    if (sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE))
    {
        if (sr & USART_SR_ORE)
            core->uart.uarts[port]->error = ERROR_OVERFLOW;
        else
        {
#if defined(STM32L0) || defined(STM32F0)
            __REG_RC32(UART_REGS[port]->RDR);
#else
            __REG_RC32(UART_REGS[port]->DR);
#endif
            if (sr & USART_SR_FE)
                core->uart.uarts[port]->error = ERROR_INVALID_FRAME;
            else if (sr & USART_SR_PE)
                core->uart.uarts[port]->error = ERROR_INVALID_PARITY;
            else if  (sr & USART_SR_NE)
                core->uart.uarts[port]->error = ERROR_LINE_NOISE;
        }
    }

    //receive data
    if (sr & USART_SR_RXNE)
    {
#if defined(STM32L0) || defined(STM32F0)
        ipc.param3 = UART_REGS[port]->RDR;
#else
        ipc.param3 = UART_REGS[port]->DR;
#endif
        ipc.process = process_iget_current();
        ipc.cmd = HAL_CMD(HAL_UART, IPC_UART_ISR_RX);
        ipc.param1 = port;
        ipc_ipost(&ipc);
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

static inline void stm32_uart_set_baudrate(CORE* core, UART_PORT port, IPC* ipc)
{
    BAUD baudrate;
    unsigned int clock;
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
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

    UART_REGS[port]->CR2 = (baudrate.stop_bits == 1 ? 0 : 2) << 12;
    UART_REGS[port]->CR3 = 0;

    if (port == UART_1 || port >= UART_6)
        clock = get_clock(core, STM32_CLOCK_APB2);
    else
        clock = get_clock(core, STM32_CLOCK_APB1);
    unsigned int mantissa, fraction;
    mantissa = (25 * clock) / (4 * (baudrate.baud));
    fraction = ((mantissa % 100) * 8 + 25)  / 50;
    mantissa = mantissa / 100;
    UART_REGS[port]->BRR = (mantissa << 4) | fraction;

    UART_REGS[port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
    UART_REGS[port]->CR3 |= USART_CR3_EIE;
}

void stm32_uart_open(CORE* core, UART_PORT port, unsigned int mode)
{
    if (core->uart.uarts[port] != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    core->uart.uarts[port] = malloc(sizeof(UART));
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
    core->uart.uarts[port]->error = ERROR_OK;
    core->uart.uarts[port]->tx_stream = INVALID_HANDLE;
    core->uart.uarts[port]->tx_handle = INVALID_HANDLE;
    core->uart.uarts[port]->rx_stream = INVALID_HANDLE;
    core->uart.uarts[port]->rx_handle = INVALID_HANDLE;
    core->uart.uarts[port]->tx_total = 0;
    core->uart.uarts[port]->tx_chunk_pos = core->uart.uarts[port]->tx_chunk_size = 0;

    if (mode & UART_TX_STREAM)
    {
        core->uart.uarts[port]->tx_stream = stream_create(UART_STREAM_SIZE);
        if (core->uart.uarts[port]->tx_stream == INVALID_HANDLE)
        {
            free(core->uart.uarts[port]);
            core->uart.uarts[port] = NULL;
            return;
        }
        core->uart.uarts[port]->tx_handle = stream_open(core->uart.uarts[port]->tx_stream);
        if (core->uart.uarts[port]->tx_handle == INVALID_HANDLE)
        {
            stream_destroy(core->uart.uarts[port]->tx_stream);
            free(core->uart.uarts[port]);
            core->uart.uarts[port] = NULL;
            return;
        }
        stream_listen(core->uart.uarts[port]->tx_stream, port, HAL_UART);
    }
    if (mode & UART_RX_STREAM)
    {
        core->uart.uarts[port]->rx_stream = stream_create(UART_STREAM_SIZE);
        if (core->uart.uarts[port]->rx_stream == INVALID_HANDLE)
        {
            stream_close(core->uart.uarts[port]->tx_handle);
            stream_destroy(core->uart.uarts[port]->tx_stream);
            free(core->uart.uarts[port]);
            core->uart.uarts[port] = NULL;
            return;
        }
        core->uart.uarts[port]->rx_handle = stream_open(core->uart.uarts[port]->rx_stream);
        if (core->uart.uarts[port]->rx_handle == INVALID_HANDLE)
        {
            stream_destroy(core->uart.uarts[port]->rx_stream);
            stream_close(core->uart.uarts[port]->tx_handle);
            stream_destroy(core->uart.uarts[port]->tx_stream);
            free(core->uart.uarts[port]);
            core->uart.uarts[port] = NULL;
            return;
        }
        core->uart.uarts[port]->rx_free = stream_get_free(core->uart.uarts[port]->rx_stream);
    }
    //power up
    if (port == UART_1 || port >= UART_6)
        RCC->APB2ENR |= 1 << UART_POWER_PINS[port];
    else
        RCC->APB1ENR |= 1 << UART_POWER_PINS[port];

    //enable core
    UART_REGS[port]->CR1 |= USART_CR1_UE;
    //enable receiver
    if (mode & UART_RX_STREAM)
        UART_REGS[port]->CR1 |= USART_CR1_RE | USART_CR1_RXNEIE;

    //enable interrupts
#if defined(STM32F0) && (UARTS_COUNT > 3)
    if (core->uart.isr3_cnt++ == 0)
#endif
    {
        irq_register(UART_VECTORS[port], stm32_uart_on_isr, (void*)core);
        NVIC_EnableIRQ(UART_VECTORS[port]);
        NVIC_SetPriority(UART_VECTORS[port], 13);
    }
}

static inline void stm32_uart_close(CORE* core, UART_PORT port)
{
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    //disable interrupts
#if defined(STM32F0) && (UARTS_COUNT > 3)
    if (--core->uart.isr3_cnt == 0)
#endif
    {
        NVIC_DisableIRQ(UART_VECTORS[port]);
        irq_unregister(UART_VECTORS[port]);
    }

    //rx
    if (core->uart.uarts[port]->rx_stream != INVALID_HANDLE)
    {
        UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
        stream_close(core->uart.uarts[port]->rx_handle);
        stream_destroy(core->uart.uarts[port]->rx_stream);
    }
    //tx
    if (core->uart.uarts[port]->tx_stream != INVALID_HANDLE)
    {
        stream_close(core->uart.uarts[port]->tx_handle);
        stream_destroy(core->uart.uarts[port]->tx_stream);
    }

    //disable core
    UART_REGS[port]->CR1 &= ~USART_CR1_UE;
    //power down
    if (port == UART_1 || port >= UART_6)
        RCC->APB2ENR &= ~(1 << UART_POWER_PINS[port]);
    else
        RCC->APB1ENR &= ~(1 << UART_POWER_PINS[port]);

    free(core->uart.uarts[port]);
    core->uart.uarts[port] = NULL;
}

static inline void stm32_uart_flush(CORE* core, UART_PORT port)
{
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    if (core->uart.uarts[port]->tx_stream != INVALID_HANDLE)
    {
        stream_flush(core->uart.uarts[port]->tx_stream);
        stream_listen(core->uart.uarts[port]->tx_stream, port, HAL_UART);
        core->uart.uarts[port]->tx_total = 0;
        __disable_irq();
        core->uart.uarts[port]->tx_chunk_pos = core->uart.uarts[port]->tx_chunk_size = 0;
        __enable_irq();
    }
    if (core->uart.uarts[port]->rx_stream != INVALID_HANDLE)
        stream_flush(core->uart.uarts[port]->rx_stream);
    core->uart.uarts[port]->error = ERROR_OK;
}

static inline HANDLE stm32_uart_get_tx_stream(CORE* core, UART_PORT port)
{
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
    return core->uart.uarts[port]->tx_stream;
}

static inline HANDLE stm32_uart_get_rx_stream(CORE* core, UART_PORT port)
{
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
    return core->uart.uarts[port]->rx_stream;
}

static inline uint16_t stm32_uart_get_last_error(CORE* core, UART_PORT port)
{
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return ERROR_OK;
    }
    return core->uart.uarts[port]->error;
}

static inline void stm32_uart_clear_error(CORE* core, UART_PORT port)
{
    if (core->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    core->uart.uarts[port]->error = ERROR_OK;
}

static inline void stm32_uart_setup_printk(CORE* core, UART_PORT port)
{
    //setup kernel printk dbg
    setup_dbg(uart_write_kernel, (void*)port);
}

void stm32_uart_write(CORE* core, UART_PORT port, unsigned int total)
{
    if (total)
        core->uart.uarts[port]->tx_total = total;
    if (core->uart.uarts[port]->tx_total == 0)
        core->uart.uarts[port]->tx_total = stream_get_size(core->uart.uarts[port]->tx_stream);
    if (core->uart.uarts[port]->tx_total)
    {
        unsigned int to_read = core->uart.uarts[port]->tx_total;
        if (core->uart.uarts[port]->tx_total > UART_TX_BUF_SIZE)
            to_read = UART_TX_BUF_SIZE;
        if (stream_read(core->uart.uarts[port]->tx_handle, core->uart.uarts[port]->tx_buf, to_read))
        {
            core->uart.uarts[port]->tx_chunk_pos = 0;
            core->uart.uarts[port]->tx_chunk_size = to_read;
            core->uart.uarts[port]->tx_total -= to_read;
            //start transaction
            UART_REGS[port]->CR1 |= USART_CR1_TE | USART_CR1_TXEIE;
        }
    }
    else
        stream_listen(core->uart.uarts[port]->tx_stream, port, HAL_UART);

}

static inline void stm32_uart_read(CORE* core, UART_PORT port, char c)
{
    //caching calls to svc
    if (core->uart.uarts[port]->rx_free == 0)
        (core->uart.uarts[port]->rx_free = stream_get_free(core->uart.uarts[port]->rx_stream));
    //if stream is full, char will be discarded
    if (core->uart.uarts[port]->rx_free)
    {
        stream_write(core->uart.uarts[port]->rx_handle, &c, 1);
        core->uart.uarts[port]->rx_free--;
    }
}

void stm32_uart_request(CORE* core, IPC* ipc)
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
        stm32_uart_open(core, port, ipc->param2);
        break;
    case IPC_CLOSE:
        stm32_uart_close(core, port);
        break;
    case IPC_UART_SET_BAUDRATE:
        stm32_uart_set_baudrate(core, port, ipc);
        break;
    case IPC_FLUSH:
        stm32_uart_flush(core, port);
        break;
    case IPC_GET_TX_STREAM:
        ipc->param2 = stm32_uart_get_tx_stream(core, port);
        break;
    case IPC_GET_RX_STREAM:
        ipc->param2 = stm32_uart_get_rx_stream(core, port);
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param2 = stm32_uart_get_last_error(core, port);
        break;
    case IPC_UART_CLEAR_ERROR:
        stm32_uart_clear_error(core, port);
        break;
    case IPC_UART_SETUP_PRINTK:
        stm32_uart_setup_printk(core, port);
        break;
    case IPC_STREAM_WRITE:
    case IPC_UART_ISR_TX:
        stm32_uart_write(core, port, ipc->param3);
        //message from kernel (or ISR), no response
        break;
    case IPC_UART_ISR_RX:
        stm32_uart_read(core, port, ipc->param3);
        //message from ISR, no response
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_uart_init(CORE *core)
{
    int i;
    for (i = 0; i < UARTS_COUNT; ++i)
        core->uart.uarts[i] = NULL;
#if defined(STM32F0) && (UARTS_COUNT > 3)
    core->uart.isr3_cnt = 0;
#endif
}

