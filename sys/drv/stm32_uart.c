/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "../sys.h"
#include "error.h"
#include "../../userspace/lib/stdlib.h"
#include "../../userspace/lib/stdio.h"
#include "../../userspace/stream.h"
#include "../../userspace/direct.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include "stm32_config.h"
#include "sys_config.h"
#include <string.h>

void stm32_uart();

typedef struct {
    PIN tx_pin, rx_pin;
    int error;
    HANDLE tx_stream, rx_stream, tx_handle, rx_handle, process;
    unsigned int tx_total;
    uint16_t tx_chunk_pos, tx_chunk_size;
    char tx_buf[UART_TX_BUF_SIZE];
    char rx_char;
    unsigned int rx_free;
} UART;

typedef struct {
    UART* uart[UARTS_COUNT];
    HANDLE process;
} UART_DRV;

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
    10,
    //function
    stm32_uart
};

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
    UART* uart = (UART*)param;
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
    if ((UART_REGS[port]->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE) && uart->tx_chunk_size)
    {
#if defined(STM32L0)
        UART_REGS[port]->TDR = uart->tx_buf[uart->tx_chunk_pos++];
#else
        UART_REGS[port]->DR = uart->tx_buf[uart->tx_chunk_pos++];
#endif
        //no more
        if (uart->tx_chunk_pos >= uart->tx_chunk_size)
        {
            uart->tx_chunk_pos = uart->tx_chunk_size = 0;
            ipc.process = uart->process;
            ipc.cmd = IPC_UART_ISR_WRITE_CHUNK_COMPLETE;
            ipc.param1 = port;
            ipc_ipost(&ipc);
            UART_REGS[port]->CR1 &= ~USART_CR1_TXEIE;
            UART_REGS[port]->CR1 |= USART_CR1_TCIE;

        }
    }
    //transmission completed and no more data. Disable transmitter
    else if ((UART_REGS[port]->CR1 & USART_CR1_TCIE) && (sr & USART_SR_TC) &&  (uart->tx_total == 0))
        UART_REGS[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
    //decode error, if any
    if ((sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)))
    {
        if (sr & USART_SR_ORE)
            uart->error = ERROR_OVERFLOW;
        else
        {
#if defined(STM32L0)
            __REG_RC32(UART_REGS[port]->RDR);
#else
            __REG_RC32(UART_REGS[port]->DR);
#endif
            if (sr & USART_SR_FE)
                uart->error = ERROR_UART_FRAME;
            else if (sr & USART_SR_PE)
                uart->error = ERROR_UART_PARITY;
            else if  (sr & USART_SR_NE)
                uart->error = ERROR_UART_NOISE;
        }
    }

    //receive data
    if (sr & USART_SR_RXNE)
    {
#if defined(STM32L0)
        uart->rx_char = UART_REGS[port]->RDR;
#else
        uart->rx_char = UART_REGS[port]->DR;
#endif
        ipc.process = uart->process;
        ipc.cmd = IPC_UART_ISR_RX_CHAR;
        ipc.param1 = (unsigned int)uart;
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

#if (SYS_INFO)
#if (UART_STDIO)
//we can't use printf in uart driver, because this can halt driver loop
static void printu(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(fmt, va, uart_write_kernel, (void*)UART_STDIO_PORT);
    va_end(va);
}
#else
#define printu printf
#endif
#endif //SYS_INFO

void stm32_uart_set_baudrate_internal(UART_DRV* drv, UART_PORT port, const BAUD* config)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    HANDLE core = object_get(SYS_OBJ_CORE);
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
        clock = get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_APB2, 0, 0);
    else
        clock = get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_APB1, 0, 0);
    unsigned int mantissa, fraction;
    mantissa = (25 * clock) / (4 * (config->baud));
    fraction = ((mantissa % 100) * 8 + 25)  / 50;
    mantissa = mantissa / 100;
    UART_REGS[port]->BRR = (mantissa << 4) | fraction;

    UART_REGS[port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
    UART_REGS[port]->CR3 |= USART_CR3_EIE;
}

void stm32_uart_set_baudrate(UART_DRV* drv, UART_PORT port, HANDLE process)
{
    BAUD baud;
    if (direct_read(process, (void*)&baud, sizeof(BAUD)))
        stm32_uart_set_baudrate_internal(drv, port, &baud);
}

void stm32_uart_open_internal(UART_DRV* drv, UART_PORT port, UART_ENABLE* ue)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    HANDLE core = object_get(SYS_OBJ_CORE);
    drv->uart[port] = malloc(sizeof(UART));
    if (drv->uart[port] == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
    drv->uart[port]->tx_pin = ue->tx;
    drv->uart[port]->rx_pin = ue->rx;
    drv->uart[port]->error = ERROR_OK;
    drv->uart[port]->tx_stream = INVALID_HANDLE;
    drv->uart[port]->tx_handle = INVALID_HANDLE;
    drv->uart[port]->rx_stream = INVALID_HANDLE;
    drv->uart[port]->rx_handle = INVALID_HANDLE;
    drv->uart[port]->tx_total = 0;
    drv->uart[port]->tx_chunk_pos = drv->uart[port]->tx_chunk_size = 0;
    drv->uart[port]->process = process_get_current();
    if (drv->uart[port]->tx_pin == PIN_DEFAULT)
        drv->uart[port]->tx_pin = UART_TX_PINS[port];
    if (drv->uart[port]->rx_pin == PIN_DEFAULT)
        drv->uart[port]->rx_pin = UART_RX_PINS[port];

    if (drv->uart[port]->tx_pin != PIN_UNUSED)
    {
        drv->uart[port]->tx_stream = stream_create(ue->tx_stream_size);
        if (drv->uart[port]->tx_stream == INVALID_HANDLE)
        {
            free(drv->uart[port]);
            drv->uart[port] = NULL;
            return;
        }
        drv->uart[port]->tx_handle = stream_open(drv->uart[port]->tx_stream);
        if (drv->uart[port]->tx_handle == INVALID_HANDLE)
        {
            stream_destroy(drv->uart[port]->tx_stream);
            free(drv->uart[port]);
            drv->uart[port] = NULL;
            return;
        }
        stream_listen(drv->uart[port]->tx_stream, (void*)port);
    }
    if (drv->uart[port]->rx_pin != PIN_UNUSED)
    {
        drv->uart[port]->rx_stream = stream_create(ue->rx_stream_size);
        if (drv->uart[port]->rx_stream == INVALID_HANDLE)
        {
            stream_close(drv->uart[port]->tx_handle);
            stream_destroy(drv->uart[port]->tx_stream);
            free(drv->uart[port]);
            drv->uart[port] = NULL;
            return;
        }
        drv->uart[port]->rx_handle = stream_open(drv->uart[port]->rx_stream);
        if (drv->uart[port]->rx_handle == INVALID_HANDLE)
        {
            stream_destroy(drv->uart[port]->rx_stream);
            stream_close(drv->uart[port]->tx_handle);
            stream_destroy(drv->uart[port]->tx_stream);
            free(drv->uart[port]);
            drv->uart[port] = NULL;
            return;
        }
        drv->uart[port]->rx_free = stream_get_free(drv->uart[port]->rx_stream);
    }

    //setup pins
#if defined(STM32F1)
    //turn on remapping
    if (((drv->uart[port]->tx_pin != UART_TX_PINS[port]) && (drv->uart[port]->tx_pin != PIN_UNUSED)) ||
        ((drv->uart[port]->rx_pin != UART_RX_PINS[port]) && (drv->uart[port]->rx_pin != PIN_UNUSED)))
    {
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        switch (drv->uart[port]->tx_pin)
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
            free(drv->uart[port]);
            drv->uart[port] = NULL;
            return;
        }
    }
#endif
    if (drv->uart[port]->tx_pin != PIN_UNUSED)
    {
#if defined(STM32F1)
        ack(core, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart[port]->tx_pin, GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
#elif defined(STM32F2) || defined(STM32F4)
        ack(core, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart[port]->tx_pin, GPIO_MODE_AF | GPIO_OT_PUSH_PULL |  GPIO_SPEED_HIGH, drv->uart[port]->port < UART_4 ? AF7 : AF8);
#elif defined(STM32L0)
        ack(core, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart[port]->tx_pin, GPIO_MODE_AF | GPIO_OT_PUSH_PULL |  GPIO_SPEED_HIGH, drv->uart[port]->tx_pin == UART_TX_PINS[port] ? AF0 : AF4);
#endif
    }

    if (drv->uart[port]->rx_pin != PIN_UNUSED)
    {
#if defined(STM32F1)
        ack(core, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart[port]->rx_pin, GPIO_MODE_INPUT_FLOAT, false);
#elif defined(STM32F2) || defined(STM32F4)
        ack(core, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart[port]->rx_pin, , GPIO_MODE_AF | GPIO_SPEED_HIGH, drv->uart[port]->port < UART_4 ? AF7 : AF8);
#elif defined(STM32L0)
        ack(core, STM32_GPIO_ENABLE_PIN_SYSTEM, drv->uart[port]->rx_pin, GPIO_MODE_AF | GPIO_SPEED_HIGH, drv->uart[port]->rx_pin == UART_RX_PINS[port] ? AF0 : AF4);
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
    if (drv->uart[port]->rx_pin != PIN_UNUSED)
        UART_REGS[port]->CR1 |= USART_CR1_RE | USART_CR1_RXNEIE;

    stm32_uart_set_baudrate_internal(drv, port, &ue->baud);
    //enable interrupts
    irq_register(UART_VECTORS[port], stm32_uart_on_isr, (void*)drv->uart[port]);
    NVIC_EnableIRQ(UART_VECTORS[port]);
    NVIC_SetPriority(UART_VECTORS[port], 13);
}

void stm32_uart_open(UART_DRV* drv, UART_PORT port, HANDLE process)
{
    UART_ENABLE ue;
    if (direct_read(process, (void*)&ue, sizeof(UART_ENABLE)))
        stm32_uart_open_internal(drv, port, &ue);
}

static inline void stm32_uart_close(UART_DRV* drv, UART_PORT port)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    HANDLE core = object_get(SYS_OBJ_CORE);
    //disable interrupts
    NVIC_DisableIRQ(UART_VECTORS[port]);
    irq_unregister(UART_VECTORS[port]);

    //disable receiver
    if (drv->uart[port]->rx_pin != PIN_UNUSED)
        UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
    //disable core
    UART_REGS[port]->CR1 &= ~USART_CR1_UE;
    //power down
    if (port == UART_1 || port == UART_6)
        RCC->APB2ENR &= ~(1 << UART_POWER_PINS[port]);
    else
        RCC->APB1ENR &= ~(1 << UART_POWER_PINS[port]);

    //disable pins
    if (drv->uart[port]->tx_pin != PIN_UNUSED)
    {
        stream_close(drv->uart[port]->tx_handle);
        stream_destroy(drv->uart[port]->tx_stream);
        ack(core, GPIO_DISABLE_PIN, drv->uart[port]->tx_pin, 0, 0);
    }
    if (drv->uart[port]->rx_pin != PIN_UNUSED)
    {
        stream_close(drv->uart[port]->rx_handle);
        stream_destroy(drv->uart[port]->rx_stream);
        ack(core, GPIO_DISABLE_PIN, drv->uart[port]->rx_pin, 0, 0);
    }

#if defined(STM32F1)
    //turn off remapping
    if (((drv->uart[port]->tx_pin != UART_TX_PINS[port]) && (drv->uart[port]->tx_pin != PIN_UNUSED)) ||
        ((drv->uart[port]->rx_pin != UART_RX_PINS[port]) && (drv->uart[port]->rx_pin != PIN_UNUSED)))
    {
        switch (drv->uart[port]->tx_pin)
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
        //if not used anymore, save some power
        if (AFIO->MAPR == 0x00000000 && AFIO->MAPR2 == 0x00000000)
            RCC->APB2ENR &= RCC_APB2ENR_AFIOEN;
    }
#endif
    free(drv->uart[port]);
    drv->uart[port] = NULL;
}

void stm32_uart_write_chunk(UART_DRV* drv, UART_PORT port)
{
    unsigned int to_read = drv->uart[port]->tx_total;
    if (drv->uart[port]->tx_total > UART_TX_BUF_SIZE)
        to_read = UART_TX_BUF_SIZE;
    if (stream_read(drv->uart[port]->tx_handle, drv->uart[port]->tx_buf, to_read))
    {
        drv->uart[port]->tx_chunk_pos = 0;
        drv->uart[port]->tx_chunk_size = to_read;
        drv->uart[port]->tx_total -= to_read;
        //start transaction
        UART_REGS[port]->CR1 |= USART_CR1_TE | USART_CR1_TXEIE;
    }
}

#if (SYS_INFO)
static inline void stm32_uart_info(UART_DRV* drv)
{
    int i;
    printu("STM32 uart driver info\n\r\n\r");
    printu("uarts count: %d\n\r", UARTS_COUNT);

    for (i = 0; i < UARTS_COUNT; ++i)
    {
        if (drv->uart[i])
            printu("UART_%d: ", i + 1);
    }
    printu("\n\r\n\r");
}
#endif //SYS_INFO

static inline void stm32_uart_loop(UART_DRV* drv)
{
    IPC ipc;
    for (;;)
    {
        error(ERROR_OK);
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case IPC_SET_STDIO:
            open_stdout();
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
            stm32_uart_info(drv);
            ipc_post(&ipc);
            break;
#endif
        case IPC_OPEN:
            stm32_uart_open(drv, ipc.param1, ipc.process);
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
            stm32_uart_close(drv, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case IPC_UART_SET_BAUDRATE:
            stm32_uart_set_baudrate(drv, ipc.param1, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case IPC_FLUSH:
            if (ipc.param1 < UARTS_COUNT && drv->uart[ipc.param1])
            {
                if (drv->uart[ipc.param1]->tx_stream != INVALID_HANDLE)
                {
                    stream_flush(drv->uart[ipc.param1]->tx_stream);
                    stream_listen(drv->uart[ipc.param1]->tx_stream, (void*)(ipc.param1));
                    drv->uart[ipc.param1]->tx_total = 0;
                    __disable_irq();
                    drv->uart[ipc.param1]->tx_chunk_pos = drv->uart[ipc.param1]->tx_chunk_size = 0;
                    __enable_irq();
                }
                if (drv->uart[ipc.param1]->rx_stream != INVALID_HANDLE)
                    stream_flush(drv->uart[ipc.param1]->rx_stream);
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_GET_TX_STREAM:
            if (ipc.param1 < UARTS_COUNT && drv->uart[ipc.param1])
            {
                ipc.param1 = drv->uart[ipc.param1]->tx_stream;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_GET_RX_STREAM:
            if (ipc.param1 < UARTS_COUNT && drv->uart[ipc.param1])
            {
                ipc.param1 = drv->uart[ipc.param1]->rx_stream;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_UART_GET_LAST_ERROR:
            if (ipc.param1 < UARTS_COUNT && drv->uart[ipc.param1])
            {
                ipc.param1 = drv->uart[ipc.param1]->error;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_UART_CLEAR_ERROR:
            if (ipc.param1 < UARTS_COUNT && drv->uart[ipc.param1])
            {
                drv->uart[ipc.param1]->error = ERROR_OK;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_INVALID_PARAMS);
            break;
        case IPC_STREAM_WRITE:
            drv->uart[ipc.param3]->tx_total = ipc.param2;
            stm32_uart_write_chunk(drv, ipc.param3);
            //message from kernel, no response
            break;
        case IPC_UART_ISR_WRITE_CHUNK_COMPLETE:
            if (drv->uart[ipc.param1]->tx_total == 0)
                drv->uart[ipc.param1]->tx_total = stream_get_size(drv->uart[ipc.param1]->tx_stream);
            if (drv->uart[ipc.param1]->tx_total)
                stm32_uart_write_chunk(drv, ipc.param1);
            else
                stream_listen(drv->uart[ipc.param1]->tx_stream, (void*)(ipc.param1));
            //message from ISR, no response
            break;
        case IPC_UART_ISR_RX_CHAR:
            //caching calls to svc
            if (((UART*)ipc.param1)->rx_free == 0)
                ((UART*)ipc.param1)->rx_free = stream_get_free(((UART*)ipc.param1)->rx_stream);
            //if stream is full, char will be discarded
            if (((UART*)ipc.param1)->rx_free)
            {
                stream_write(((UART*)ipc.param1)->rx_handle, &((UART*)ipc.param1)->rx_char, 1);
                ((UART*)ipc.param1)->rx_free--;
            }
            //message from ISR, no response
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

#if (UART_STDIO)
static inline void stm32_uart_open_stdio(UART_DRV* drv)
{
    UART_ENABLE ue;
    ue.tx = UART_STDIO_TX;
    ue.rx = UART_STDIO_RX;
    ue.baud.data_bits = UART_STDIO_DATA_BITS;
    ue.baud.parity = UART_STDIO_PARITY;
    ue.baud.stop_bits = UART_STDIO_STOP_BITS;
    ue.baud.baud = UART_STDIO_BAUD;
    ue.tx_stream_size = STDIO_TX_STREAM_SIZE;
    ue.rx_stream_size = STDIO_RX_STREAM_SIZE;
    stm32_uart_open_internal(drv, UART_STDIO_PORT, &ue);

    //setup kernel printk dbg
    setup_dbg(uart_write_kernel, (void*)UART_STDIO_PORT);
    //setup system stdio
    object_set(SYS_OBJ_STDOUT, drv->uart[UART_STDIO_PORT]->tx_stream);
    object_set(SYS_OBJ_STDIN, drv->uart[UART_STDIO_PORT]->rx_stream);
#if (SYS_INFO)
    //inform early process (core) about stdio.
    ack(object_get(SYS_OBJ_CORE), IPC_SET_STDIO, 0, 0, 0);
#endif //SYS_INFO
}
#endif

void stm32_uart()
{
    UART_DRV drv;
    memset(&drv, 0, sizeof(UART_DRV));
    object_set_self(SYS_OBJ_UART);

#if (UART_STDIO)
    stm32_uart_open_stdio(&drv);
#endif //UART_STDIO

    stm32_uart_loop(&drv);
}
