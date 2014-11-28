/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "../../sys.h"
#include "error.h"
#include "../../../userspace/lib/stdlib.h"
#include "../../../userspace/lib/stdio.h"
#include "../../../userspace/stream.h"
#include "../../../userspace/direct.h"
#include "../../../userspace/irq.h"
#include "../../../userspace/object.h"
#include <string.h>
#if (MONOLITH_UART)
#include "stm32_core_private.h"


#define get_clock               stm32_power_get_clock_inside
#define ack_gpio                stm32_gpio_request_inside

#else

#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside

void stm32_uart();

const REX __STM32_UART = {
    //name
    "STM32 uart",
    //size
    STM32_UART_STACK_SIZE,
    //priority - driver priority. Setting priority lower than other drivers can cause IPC overflow on SYS_INFO
    89,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_DRIVERS_IPC_COUNT,
    //function
    stm32_uart
};
#endif

typedef USART_TypeDef* USART_TypeDef_P;

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2};

#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3};

#elif defined(STM32F1)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19, 20};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5};

#elif defined(STM32F2) || defined(STM32F40_41xxx)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6};

#elif defined(STM32F4)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71, 82, 83};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5, 30, 31};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6, F7, F1};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7, F6, E0};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6, UART7, UART8};
#elif defined(STM32L0)
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {27, 28};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {B6, PIN_UNUSED};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {B7, PIN_UNUSED};
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
            ipc.cmd = IPC_UART_ISR_TX;
            ipc.param2 = 0;
            ipc.param3 = HAL_HANDLE(HAL_UART, port);
            ipc_ipost(&ipc);
            UART_REGS[port]->CR1 &= ~USART_CR1_TXEIE;
            UART_REGS[port]->CR1 |= USART_CR1_TCIE;

        }
    }
    //transmission completed and no more data. Disable transmitter
    else if ((UART_REGS[port]->CR1 & USART_CR1_TCIE) && (sr & USART_SR_TC) &&  (drv->uart.uarts[port]->tx_total == 0))
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
        ipc.param1 = UART_REGS[port]->RDR;
#else
        ipc.param1 = UART_REGS[port]->DR;
#endif
        ipc.process = process_iget_current();
        ipc.cmd = IPC_UART_ISR_RX;
        ipc.param3 = HAL_HANDLE(HAL_UART, port);
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

#if (SYS_INFO) && (UART_STDIO)
//we can't use printf in uart driver, because this can halt driver loop
void printu(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(fmt, va, uart_write_kernel, (void*)UART_STDIO_PORT);
    va_end(va);
}
#endif //(SYS_INFO) && (UART_STDIO)

void stm32_uart_set_baudrate_internal(SHARED_UART_DRV* drv, UART_PORT port, const BAUD* config)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    unsigned int clock;
    UART_REGS[port]->CR1 &= ~USART_CR1_UE;

    if (config->data_bits == 8 && config->parity != 'N')
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

    if (config->parity != 'N')
    {
        UART_REGS[port]->CR1 |= USART_CR1_PCE;
        if (config->parity == 'O')
            UART_REGS[port]->CR1 |= USART_CR1_PS;
        else
            UART_REGS[port]->CR1 &= ~USART_CR1_PS;
    }
    else
        UART_REGS[port]->CR1 &= ~USART_CR1_PCE;

    UART_REGS[port]->CR2 = (config->stop_bits == 1 ? 0 : 2) << 12;
    UART_REGS[port]->CR3 = 0;

    if (port == UART_1 || port == UART_6)
        clock = get_clock(drv, STM32_CLOCK_APB2);
    else
        clock = get_clock(drv, STM32_CLOCK_APB1);
    unsigned int mantissa, fraction;
    mantissa = (25 * clock) / (4 * (config->baud));
    fraction = ((mantissa % 100) * 8 + 25)  / 50;
    mantissa = mantissa / 100;
    UART_REGS[port]->BRR = (mantissa << 4) | fraction;

    UART_REGS[port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
    UART_REGS[port]->CR3 |= USART_CR3_EIE;
}

void stm32_uart_set_baudrate(SHARED_UART_DRV* drv, UART_PORT port, HANDLE process)
{
    BAUD baud;
    if (direct_read(process, (void*)&baud, sizeof(BAUD)))
        stm32_uart_set_baudrate_internal(drv, port, &baud);
}

void stm32_uart_open_internal(SHARED_UART_DRV* drv, UART_PORT port, UART_ENABLE* ue)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
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
    drv->uart.uarts[port]->tx_pin = ue->tx;
    drv->uart.uarts[port]->rx_pin = ue->rx;
    drv->uart.uarts[port]->error = ERROR_OK;
    drv->uart.uarts[port]->tx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_total = 0;
    drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
    if (drv->uart.uarts[port]->tx_pin == PIN_DEFAULT)
        drv->uart.uarts[port]->tx_pin = UART_TX_PINS[port];
    if (drv->uart.uarts[port]->rx_pin == PIN_DEFAULT)
        drv->uart.uarts[port]->rx_pin = UART_RX_PINS[port];

    if (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)
    {
        drv->uart.uarts[port]->tx_stream = stream_create(ue->stream_size);
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
        stream_listen(drv->uart.uarts[port]->tx_stream, (void*)HAL_HANDLE(HAL_UART, port));
    }
    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
    {
        drv->uart.uarts[port]->rx_stream = stream_create(ue->stream_size);
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

    //setup pins
#if defined(STM32F1)
    //turn on remapping
    if (((drv->uart.uarts[port]->tx_pin != UART_TX_PINS[port]) && (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)) ||
        ((drv->uart.uarts[port]->rx_pin != UART_RX_PINS[port]) && (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)))
    {
        ack_gpio(drv, STM32_GPIO_ENABLE_AFIO, 0, 0, 0);
        switch (drv->uart.uarts[port]->tx_pin)
        {
        case B6:
            AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;
            break;
        case D5:
            AFIO->MAPR |= AFIO_MAPR_USART2_REMAP;
            break;
        case C10:
            AFIO->MAPR |= AFIO_MAPR_USART3_REMAP_PARTIALREMAP;
            break;
        case D8:
            AFIO->MAPR |= AFIO_MAPR_USART3_REMAP_FULLREMAP;
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            free(drv->uart.uarts[port]);
            drv->uart.uarts[port] = NULL;
            return;
        }
    }
#endif
    if (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)
    {
#if defined(STM32F1)
        ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->tx_pin, GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
#elif defined(STM32F2) || defined(STM32F4)
        ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->tx_pin, GPIO_MODE_AF | GPIO_OT_PUSH_PULL |  GPIO_SPEED_HIGH, drv->uart.uarts[port]->port < UART_4 ? AF7 : AF8);
#elif defined(STM32L0)
        ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->tx_pin, GPIO_MODE_AF | GPIO_OT_PUSH_PULL |  GPIO_SPEED_HIGH, drv->uart.uarts[port]->tx_pin == UART_TX_PINS[port] ? AF0 : AF4);
#endif
    }

    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
    {
#if defined(STM32F1)
        ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->rx_pin, GPIO_MODE_INPUT_FLOAT, false);
#elif defined(STM32F2) || defined(STM32F4)
        ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->rx_pin, , GPIO_MODE_AF | GPIO_SPEED_HIGH, drv->uart.uarts[port]->port < UART_4 ? AF7 : AF8);
#elif defined(STM32L0)
        ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->rx_pin, GPIO_MODE_AF | GPIO_SPEED_HIGH, drv->uart.uarts[port]->rx_pin == UART_RX_PINS[port] ? AF0 : AF4);
#endif
    }
    //power up
    if (port == UART_1 || port == UART_6)
        RCC->APB2ENR |= 1 << UART_POWER_PINS[port];
    else
        RCC->APB1ENR |= 1 << UART_POWER_PINS[port];

    //enable core
    UART_REGS[port]->CR1 |= USART_CR1_UE;
    //enable receiver
    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
        UART_REGS[port]->CR1 |= USART_CR1_RE | USART_CR1_RXNEIE;

    stm32_uart_set_baudrate_internal(drv, port, &ue->baud);
    //enable interrupts
    irq_register(UART_VECTORS[port], stm32_uart_on_isr, (void*)drv);
    NVIC_EnableIRQ(UART_VECTORS[port]);
    NVIC_SetPriority(UART_VECTORS[port], 13);
}

void stm32_uart_open(SHARED_UART_DRV* drv, UART_PORT port, HANDLE process)
{
    UART_ENABLE ue;
    if (direct_read(process, (void*)&ue, sizeof(UART_ENABLE)))
        stm32_uart_open_internal(drv, port, &ue);
}

static inline void stm32_uart_close(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    //disable interrupts
    NVIC_DisableIRQ(UART_VECTORS[port]);
    irq_unregister(UART_VECTORS[port]);

    //disable receiver
    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
        UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
    //disable core
    UART_REGS[port]->CR1 &= ~USART_CR1_UE;
    //power down
    if (port == UART_1 || port == UART_6)
        RCC->APB2ENR &= ~(1 << UART_POWER_PINS[port]);
    else
        RCC->APB1ENR &= ~(1 << UART_POWER_PINS[port]);

    //disable pins
    if (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)
    {
        stream_close(drv->uart.uarts[port]->tx_handle);
        stream_destroy(drv->uart.uarts[port]->tx_stream);
        ack_gpio(drv, GPIO_DISABLE_PIN, drv->uart.uarts[port]->tx_pin, 0, 0);
    }
    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
    {
        stream_close(drv->uart.uarts[port]->rx_handle);
        stream_destroy(drv->uart.uarts[port]->rx_stream);
        ack_gpio(drv, GPIO_DISABLE_PIN, drv->uart.uarts[port]->rx_pin, 0, 0);
    }

#if defined(STM32F1)
    //turn off remapping
    if (((drv->uart.uarts[port]->tx_pin != UART_TX_PINS[port]) && (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)) ||
        ((drv->uart.uarts[port]->rx_pin != UART_RX_PINS[port]) && (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)))
    {
        switch (drv->uart.uarts[port]->tx_pin)
        {
        case B6:
            AFIO->MAPR &= ~AFIO_MAPR_USART1_REMAP;
            break;
        case D5:
            AFIO->MAPR &= ~AFIO_MAPR_USART2_REMAP;
            break;
        case C10:
            AFIO->MAPR &= ~AFIO_MAPR_USART3_REMAP_PARTIALREMAP;
            break;
        case D8:
            AFIO->MAPR &= ~AFIO_MAPR_USART3_REMAP_FULLREMAP;
            break;
        default:
            break;
        }
        ack_gpio(drv, STM32_GPIO_DISABLE_AFIO, 0, 0, 0);
    }
#endif
    free(drv->uart.uarts[port]);
    drv->uart.uarts[port] = NULL;
}

static inline void stm32_uart_flush(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    if (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)
    {
        stream_flush(drv->uart.uarts[port]->tx_stream);
        stream_listen(drv->uart.uarts[port]->tx_stream, (void*)HAL_HANDLE(HAL_UART, port));
        drv->uart.uarts[port]->tx_total = 0;
        __disable_irq();
        drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
        __enable_irq();
    }
    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
        stream_flush(drv->uart.uarts[port]->rx_stream);
    drv->uart.uarts[port]->error = ERROR_OK;
}

static inline HANDLE stm32_uart_get_tx_stream(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return INVALID_HANDLE;
    }
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
    return drv->uart.uarts[port]->tx_stream;
}

static inline HANDLE stm32_uart_get_rx_stream(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return INVALID_HANDLE;
    }
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return INVALID_HANDLE;
    }
    if (drv->uart.uarts[port]->rx_pin == PIN_UNUSED)
    {
        error(ERROR_NOT_CONFIGURED);
        return INVALID_HANDLE;
    }
    return drv->uart.uarts[port]->rx_stream;
}

static inline uint16_t stm32_uart_get_last_error(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return ERROR_OK;
    }
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return ERROR_OK;
    }
    return drv->uart.uarts[port]->error;
}

static inline void stm32_uart_clear_error(SHARED_UART_DRV* drv, UART_PORT port)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    drv->uart.uarts[port]->error = ERROR_OK;
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
        stream_listen(drv->uart.uarts[port]->tx_stream, (void*)HAL_HANDLE(HAL_UART, port));

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

#if (SYS_INFO)
static inline void stm32_uart_info(SHARED_UART_DRV* drv)
{
    int i;
    printu("STM32 uart driver info\n\r\n\r");
    printu("uarts count: %d\n\r", UARTS_COUNT);

    for (i = 0; i < UARTS_COUNT; ++i)
    {
        if (drv->uart.uarts[i])
            printu("UART_%d ", i + 1);
    }
    printu("\n\r\n\r");
}
#endif //SYS_INFO

bool stm32_uart_request(SHARED_UART_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        stm32_uart_info(drv);
        need_post = true;
        break;
#endif
    case IPC_OPEN:
        stm32_uart_open(drv, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_uart_close(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_SET_BAUDRATE:
        stm32_uart_set_baudrate(drv, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case IPC_FLUSH:
        stm32_uart_flush(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_GET_TX_STREAM:
        ipc->param1 = stm32_uart_get_tx_stream(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_GET_RX_STREAM:
        ipc->param1 = stm32_uart_get_rx_stream(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param1 = stm32_uart_get_last_error(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_CLEAR_ERROR:
        stm32_uart_clear_error(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_STREAM_WRITE:
    case IPC_UART_ISR_TX:
        stm32_uart_write(drv, HAL_ITEM(ipc->param3), ipc->param2);
        //message from kernel (or ISR), no response
        break;
    case IPC_UART_ISR_RX:
        stm32_uart_read(drv, HAL_ITEM(ipc->param3), ipc->param1);
        //message from ISR, no response
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

#if (UART_STDIO)
static inline void stm32_uart_open_stdio(SHARED_UART_DRV* drv)
{
    UART_ENABLE ue;
    ue.tx = UART_STDIO_TX;
    ue.rx = UART_STDIO_RX;
    ue.baud.data_bits = UART_STDIO_DATA_BITS;
    ue.baud.parity = UART_STDIO_PARITY;
    ue.baud.stop_bits = UART_STDIO_STOP_BITS;
    ue.baud.baud = UART_STDIO_BAUD;
    ue.stream_size = STDIO_STREAM_SIZE;
    stm32_uart_open_internal(drv, UART_STDIO_PORT, &ue);

    //setup kernel printk dbg
    setup_dbg(uart_write_kernel, (void*)UART_STDIO_PORT);
    //setup system stdio
    object_set(SYS_OBJ_STDOUT, drv->uart.uarts[UART_STDIO_PORT]->tx_stream);
#if (UART_STDIO_RX != PIN_UNUSED)
    object_set(SYS_OBJ_STDIN, drv->uart.uarts[UART_STDIO_PORT]->rx_stream);
#endif
#if (SYS_INFO) && !(MONOLITH_UART)
    //inform early process (core) about stdio.
    ack(object_get(SYS_OBJ_CORE), IPC_SET_STDIO, 0, 0, 0);
#endif //SYS_INFO && !MONOLITH_UART
}
#endif

void stm32_uart_init(SHARED_UART_DRV* drv)
{
    int i;
    for (i = 0; i < UARTS_COUNT; ++i)
        drv->uart.uarts[i] = NULL;
#if (UART_STDIO)
    stm32_uart_open_stdio(drv);
#endif //UART_STDIO
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
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        case IPC_CALL_ERROR:
            break;
        case IPC_SET_STDIO:
            open_stdout();
            need_post = true;
            break;
        default:
            need_post = stm32_uart_request(&drv, &ipc);
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
#endif
