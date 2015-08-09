/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "../../userspace/stm32_driver.h"
#include "error.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stream.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include <string.h>
#if (MONOLITH_UART)
#include "stm32_core_private.h"

#define get_clock               stm32_power_get_clock_inside
#define ack_pin                 stm32_pin_request_inside

#else

#define get_clock               stm32_power_get_clock_outside
#define ack_pin                 stm32_core_request_outside

void stm32_uart();

const REX __STM32_UART = {
    //name
    "STM32 uart",
    //size
    STM32_UART_PROCESS_SIZE,
    //priority - driver priority.
    89,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    stm32_uart
};
#endif

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
#endif

void stm32_uart_on_isr(int vector, void* param)
{
    IPC ipc;
    //find port by vector
    UART_PORT port;
    for (port = UART_1; port < UART_MAX && UART_VECTORS[port] != vector; ++port) {}
    SHARED_UART_DRV* drv = (SHARED_UART_DRV*)param;
#if defined(STM32L0)
#define USART_SR_TXE        USART_ISR_TXE
#define USART_SR_TC         USART_ISR_TC
#define USART_SR_PE         USART_ISR_PE
#define USART_SR_FE         USART_ISR_FE
#define USART_SR_NE         USART_ISR_NE
#define USART_SR_ORE        USART_ISR_ORE
#define USART_SR_RXNE       USART_ISR_RXNE
    unsigned int sr = UART_REGS[port]->ISR;
#else
    uint16_t sr = UART_REGS[port]->SR;
#endif

    //transmit more
    if ((UART_REGS[port]->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE) && drv->uart.uarts[port]->tx_chunk_size)
    {
#if defined(STM32L0)
        UART_REGS[port]->TDR = drv->uart.uarts[port]->tx_buf[drv->uart.uarts[port]->tx_chunk_pos++];
#else
        UART_REGS[port]->DR = drv->uart.uarts[port]->tx_buf[drv->uart.uarts[port]->tx_chunk_pos++];
#endif
        //no more
        if (drv->uart.uarts[port]->tx_chunk_pos >= drv->uart.uarts[port]->tx_chunk_size)
        {
            drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
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
    if ((sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)))
    {
        if (sr & USART_SR_ORE)
            drv->uart.uarts[port]->error = ERROR_OVERFLOW;
        else
        {
#if defined(STM32L0)
            __REG_RC32(UART_REGS[port]->RDR);
#else
            __REG_RC32(UART_REGS[port]->DR);
#endif
            if (sr & USART_SR_FE)
                drv->uart.uarts[port]->error = ERROR_UART_FRAME;
            else if (sr & USART_SR_PE)
                drv->uart.uarts[port]->error = ERROR_UART_PARITY;
            else if  (sr & USART_SR_NE)
                drv->uart.uarts[port]->error = ERROR_UART_NOISE;
        }
    }

    //receive data
    if (sr & USART_SR_RXNE)
    {
#if defined(STM32L0)
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
#if defined(STM32L0)
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

static inline void stm32_uart_set_baudrate(SHARED_UART_DRV* drv, UART_PORT port, IPC* ipc)
{
    BAUD baudrate;
    unsigned int clock;
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    uart_decode_baudrate(ipc, &baudrate);

    UART_REGS[port]->CR1 &= ~USART_CR1_UE;

    if (baudrate.data_bits == 8 && baudrate.parity != 'N')
#if defined(STM32L0)
        UART_REGS[port]->CR1 |= USART_CR1_M_0;
#else
        UART_REGS[port]->CR1 |= USART_CR1_M;
#endif
    else
#if defined(STM32L0)
        UART_REGS[port]->CR1 &= ~USART_CR1_M_0;
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

    if (port == UART_1 || port == UART_6)
        clock = get_clock(drv, STM32_CLOCK_APB2);
    else
        clock = get_clock(drv, STM32_CLOCK_APB1);
    unsigned int mantissa, fraction;
    mantissa = (25 * clock) / (4 * (baudrate.baud));
    fraction = ((mantissa % 100) * 8 + 25)  / 50;
    mantissa = mantissa / 100;
    UART_REGS[port]->BRR = (mantissa << 4) | fraction;

    UART_REGS[port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
    UART_REGS[port]->CR3 |= USART_CR3_EIE;
}

void stm32_uart_open(SHARED_UART_DRV* drv, UART_PORT port, unsigned int mode)
{
    if (drv->uart.uarts[port] != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    drv->uart.uarts[port] = malloc(sizeof(UART));
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
    drv->uart.uarts[port]->error = ERROR_OK;
    drv->uart.uarts[port]->tx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_total = 0;
    drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;

    if (mode & UART_TX_STREAM)
    {
        drv->uart.uarts[port]->tx_stream = stream_create(UART_STREAM_SIZE);
        if (drv->uart.uarts[port]->tx_stream == INVALID_HANDLE)
        {
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        drv->uart.uarts[port]->tx_handle = stream_open(drv->uart.uarts[port]->tx_stream);
        if (drv->uart.uarts[port]->tx_handle == INVALID_HANDLE)
        {
            stream_destroy(drv->uart.uarts[port]->tx_stream);
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        stream_listen(drv->uart.uarts[port]->tx_stream, port, HAL_UART);
    }
    if (mode & UART_RX_STREAM)
    {
        drv->uart.uarts[port]->rx_stream = stream_create(UART_STREAM_SIZE);
        if (drv->uart.uarts[port]->rx_stream == INVALID_HANDLE)
        {
            stream_close(drv->uart.uarts[port]->tx_handle);
            stream_destroy(drv->uart.uarts[port]->tx_stream);
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        drv->uart.uarts[port]->rx_handle = stream_open(drv->uart.uarts[port]->rx_stream);
        if (drv->uart.uarts[port]->rx_handle == INVALID_HANDLE)
        {
            stream_destroy(drv->uart.uarts[port]->rx_stream);
            stream_close(drv->uart.uarts[port]->tx_handle);
            stream_destroy(drv->uart.uarts[port]->tx_stream);
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
        drv->uart.uarts[port]->rx_free = stream_get_free(drv->uart.uarts[port]->rx_stream);
    }
    //power up
    if (port == UART_1 || port == UART_6)
        RCC->APB2ENR |= 1 << UART_POWER_PINS[port];
    else
        RCC->APB1ENR |= 1 << UART_POWER_PINS[port];

    //enable core
    UART_REGS[port]->CR1 |= USART_CR1_UE;
    //enable receiver
    if (mode & UART_RX_STREAM)
        UART_REGS[port]->CR1 |= USART_CR1_RE | USART_CR1_RXNEIE;

    //enable interrupts
    irq_register(UART_VECTORS[port], stm32_uart_on_isr, (void*)drv);
    NVIC_EnableIRQ(UART_VECTORS[port]);
    NVIC_SetPriority(UART_VECTORS[port], 13);
}

static inline void stm32_uart_close(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    //disable interrupts
    NVIC_DisableIRQ(UART_VECTORS[port]);
    irq_unregister(UART_VECTORS[port]);

    //rx
    if (drv->uart.uarts[port]->rx_stream != INVALID_HANDLE)
    {
        UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
        stream_close(drv->uart.uarts[port]->rx_handle);
        stream_destroy(drv->uart.uarts[port]->rx_stream);
    }
    //tx
    if (drv->uart.uarts[port]->tx_stream != INVALID_HANDLE)
    {
        stream_close(drv->uart.uarts[port]->tx_handle);
        stream_destroy(drv->uart.uarts[port]->tx_stream);
    }

    //disable core
    UART_REGS[port]->CR1 &= ~USART_CR1_UE;
    //power down
    if (port == UART_1 || port == UART_6)
        RCC->APB2ENR &= ~(1 << UART_POWER_PINS[port]);
    else
        RCC->APB1ENR &= ~(1 << UART_POWER_PINS[port]);

    free(drv->uart.uarts[port]);
    drv->uart.uarts[port] = NULL;
}

static inline void stm32_uart_flush(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    if (drv->uart.uarts[port]->tx_stream != INVALID_HANDLE)
    {
        stream_flush(drv->uart.uarts[port]->tx_stream);
        stream_listen(drv->uart.uarts[port]->tx_stream, port, HAL_UART);
        drv->uart.uarts[port]->tx_total = 0;
        __disable_irq();
        drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
        __enable_irq();
    }
    if (drv->uart.uarts[port]->rx_stream != INVALID_HANDLE)
        stream_flush(drv->uart.uarts[port]->rx_stream);
    drv->uart.uarts[port]->error = ERROR_OK;
}

static inline HANDLE stm32_uart_get_tx_stream(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
    return drv->uart.uarts[port]->tx_stream;
}

static inline HANDLE stm32_uart_get_rx_stream(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
    return drv->uart.uarts[port]->rx_stream;
}

static inline uint16_t stm32_uart_get_last_error(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return ERROR_OK;
    }
    return drv->uart.uarts[port]->error;
}

static inline void stm32_uart_clear_error(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    drv->uart.uarts[port]->error = ERROR_OK;
}

static inline void stm32_uart_setup_printk(SHARED_UART_DRV* drv, UART_PORT port)
{
    //setup kernel printk dbg
    setup_dbg(uart_write_kernel, (void*)port);
}

void stm32_uart_write(SHARED_UART_DRV* drv, UART_PORT port, unsigned int total)
{
    if (total)
        drv->uart.uarts[port]->tx_total = total;
    if (drv->uart.uarts[port]->tx_total == 0)
        drv->uart.uarts[port]->tx_total = stream_get_size(drv->uart.uarts[port]->tx_stream);
    if (drv->uart.uarts[port]->tx_total)
    {
        unsigned int to_read = drv->uart.uarts[port]->tx_total;
        if (drv->uart.uarts[port]->tx_total > UART_TX_BUF_SIZE)
            to_read = UART_TX_BUF_SIZE;
        if (stream_read(drv->uart.uarts[port]->tx_handle, drv->uart.uarts[port]->tx_buf, to_read))
        {
            drv->uart.uarts[port]->tx_chunk_pos = 0;
            drv->uart.uarts[port]->tx_chunk_size = to_read;
            drv->uart.uarts[port]->tx_total -= to_read;
            //start transaction
            UART_REGS[port]->CR1 |= USART_CR1_TE | USART_CR1_TXEIE;
        }
    }
    else
        stream_listen(drv->uart.uarts[port]->tx_stream, port, HAL_UART);

}

static inline void stm32_uart_read(SHARED_UART_DRV* drv, UART_PORT port, char c)
{
    //caching calls to svc
    if (drv->uart.uarts[port]->rx_free == 0)
        (drv->uart.uarts[port]->rx_free = stream_get_free(drv->uart.uarts[port]->rx_stream));
    //if stream is full, char will be discarded
    if (drv->uart.uarts[port]->rx_free)
    {
        stream_write(drv->uart.uarts[port]->rx_handle, &c, 1);
        drv->uart.uarts[port]->rx_free--;
    }
}

bool stm32_uart_request(SHARED_UART_DRV* drv, IPC* ipc)
{
    UART_PORT port = (UART_PORT)ipc->param1;
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return true;
    }
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_uart_open(drv, port, ipc->param2);
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_uart_close(drv, port);
        need_post = true;
        break;
    case IPC_UART_SET_BAUDRATE:
        stm32_uart_set_baudrate(drv, port, ipc);
        need_post = true;
        break;
    case IPC_FLUSH:
        stm32_uart_flush(drv, port);
        need_post = true;
        break;
    case IPC_GET_TX_STREAM:
        ipc->param2 = stm32_uart_get_tx_stream(drv, port);
        need_post = true;
        break;
    case IPC_GET_RX_STREAM:
        ipc->param2 = stm32_uart_get_rx_stream(drv, port);
        need_post = true;
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param2 = stm32_uart_get_last_error(drv, port);
        need_post = true;
        break;
    case IPC_UART_CLEAR_ERROR:
        stm32_uart_clear_error(drv, port);
        need_post = true;
        break;
    case IPC_UART_SETUP_PRINTK:
        stm32_uart_setup_printk(drv, port);
        need_post = true;
        break;
    case IPC_STREAM_WRITE:
    case IPC_UART_ISR_TX:
        stm32_uart_write(drv, port, ipc->param3);
        //message from kernel (or ISR), no response
        break;
    case IPC_UART_ISR_RX:
        stm32_uart_read(drv, port, ipc->param3);
        //message from ISR, no response
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void stm32_uart_init(SHARED_UART_DRV* drv)
{
    int i;
    for (i = 0; i < UARTS_COUNT; ++i)
        drv->uart.uarts[i] = NULL;
}

#if !(MONOLITH_UART)
void stm32_uart()
{
    SHARED_UART_DRV drv;
    IPC ipc;
    bool need_post;
    object_set_self(SYS_OBJ_UART);
    stm32_uart_init(&drv);
    for (;;)
    {
        ipc_read(&ipc);
        need_post = stm32_uart_request(&drv, &ipc);
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
#endif
