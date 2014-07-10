/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "../sys_call.h"
#include "error.h"
#include "../../userspace/lib/stdlib.h"
#include "stm32_config.h"

void stm32_uart();

typedef struct {
    UART_PORT port;
    PIN tx_pin, rx_pin;
    int flags;
    int error;
    HANDLE tx_stream, rx_stream;
    char tx_buf[UART_TX_BUF_SIZE];
} UART;

const REX __STM32_UART = {
    //name
    "STM32 uart",
    //size
    UART_PROCESS_SIZE,
    //priority - driver priority
    91,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_uart
};

typedef USART_TypeDef* USART_TypeDef_P;

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
#define UARTS_COUNT                                         2
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2};

#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL)
#define UARTS_COUNT                                         3
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3};

#elif defined(STM32F1)
#define UARTS_COUNT                                         5
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19, 20};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5};

#elif defined(STM32F2) || defined(STM32F40_41xxx)
#define UARTS_COUNT                                         6
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6};

#elif defined(STM32F4)
#define UARTS_COUNT                                         8
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71, 82, 83};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5, 30, 31};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6, F7, F1};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7, F6, E0};
static const USART_TypeDef_P UART_REGS[UARTS_COUNT]=        {USART1, USART2, USART3, UART4, UART5, USART6, UART7, UART8};
#endif

#define UART_ERROR_MASK                                    0xf

#define DISABLE_RECEIVER()                                UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE)
#define ENABLE_RECEIVER()                                UART_REGS[port]->CR1 |= (USART_CR1_RE | USART_CR1_RXNEIE)
#define ENABLE_TRANSMITTER()                            UART_REGS[port]->CR1 |= USART_CR1_TE
#define DISABLE_TRANSMITTER()                            UART_REGS[port]->CR1 &= ~(USART_CR1_TE)

/*
void uart_on_isr(UART_PORT port)
{
    if (__KERNEL->uart_handlers[port])
    {
        __KERNEL->uart_handlers[port]->isr_active = true;
        uint16_t sr = UART_REGS[port]->SR;

        //slave: transmit more
        if ((sr & USART_SR_TXE) && __KERNEL->uart_handlers[port]->write_size)
        {
            UART_REGS[port]->DR = __KERNEL->uart_handlers[port]->write_buf[0];
            __KERNEL->uart_handlers[port]->write_buf++;
            __KERNEL->uart_handlers[port]->write_size--;
            if (__KERNEL->uart_handlers[port]->write_size == 0)
            {
                __KERNEL->uart_handlers[port]->write_buf = NULL;
//                if (__KERNEL->uart_handlers[port]->cb->on_write_complete)
//                    __KERNEL->uart_handlers[port]->cb->on_write_complete(__KERNEL->uart_handlers[port]->param);
            }
            //no more
            if (__KERNEL->uart_handlers[port]->write_size == 0)
            {
                UART_REGS[port]->CR1 &= ~USART_CR1_TXEIE;
                UART_REGS[port]->CR1 |= USART_CR1_TCIE;
            }
        }
        //transmission completed and no more data
        else if ((sr & USART_SR_TC) && __KERNEL->uart_handlers[port]->write_size == 0)
            UART_REGS[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
        //decode error, if any
        if ((sr & UART_ERROR_MASK))
        {
//            UART_ERROR error = UART_ERROR_OK;
//            if (sr & USART_SR_ORE)
//                error = UART_ERROR_OVERRUN;
//            else
//            {
                __REG_RC32(UART_REGS[port]->DR);
//                if (sr & USART_SR_FE)
//                    error = UART_ERROR_FRAME;
//                else if (sr & USART_SR_PE)
//                    error = UART_ERROR_PARITY;
//                else if  (sr & USART_SR_NE)
//                    error = UART_ERROR_NOISE;
//            }
//            if (__KERNEL->uart_handlers[port]->cb->on_error)
//                __KERNEL->uart_handlers[port]->cb->on_error(__KERNEL->uart_handlers[port]->param, error);
        }
        //slave: receive data
        if (sr & USART_SR_RXNE && __KERNEL->uart_handlers[port]->read_size)
        {
            __KERNEL->uart_handlers[port]->read_buf[0] = UART_REGS[port]->DR;
            __KERNEL->uart_handlers[port]->read_buf++;
            if (--__KERNEL->uart_handlers[port]->read_size == 0)
            {
                __KERNEL->uart_handlers[port]->read_buf = NULL;
//                if (__KERNEL->uart_handlers[port]->cb->on_read_complete)
//                    __KERNEL->uart_handlers[port]->cb->on_read_complete(__KERNEL->uart_handlers[port]->param);
            }
            //no more, disable receiver
            if (__KERNEL->uart_handlers[port]->read_size == 0)
                UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
        }
        else if (sr & USART_SR_RXNE)
        {
            __REG_RC32(UART_REGS[port]->DR);
            UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
        }
    }
}

void uart_read(UART_PORT port, char* buf, int size)
{
    //must be handled be upper layer
    ASSERT(__KERNEL->uart_handlers[port]->read_size == 0);

    __KERNEL->uart_handlers[port]->read_buf = buf;
    __KERNEL->uart_handlers[port]->read_size = size;

    UART_REGS[port]->CR1 |= (USART_CR1_RE | USART_CR1_RXNEIE);
}
*/

void uart_read_cancel(UART_PORT port)
{
//    __KERNEL->uart_handlers[port]->read_size = 0;
//    __KERNEL->uart_handlers[port]->read_buf = NULL;
    UART_REGS[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
}

/*void uart_write_wait(UART_PORT port)
{
    CRITICAL_ENTER;
    __KERNEL->uart_handlers[port]->isr_active = false;
    while (__KERNEL->uart_handlers[port]->write_size != 0)
    {
        while ((UART_REGS[port]->SR & USART_SR_TXE) == 0) {}
//        CRITICAL_ENTER;
        //not handled in isr, do it manually
        if ((UART_REGS[port]->SR & USART_SR_TXE) && !__KERNEL->uart_handlers[port]->isr_active)
        {
            UART_REGS[port]->DR = __KERNEL->uart_handlers[port]->write_buf[0];
            __KERNEL->uart_handlers[port]->write_buf++;
            __KERNEL->uart_handlers[port]->write_size--;
        }
//        CRITICAL_LEAVE;
    }
    if (!__KERNEL->uart_handlers[port]->isr_active)
    {
//        CRITICAL_ENTER;
        __KERNEL->uart_handlers[port]->write_buf = NULL;
//        CRITICAL_LEAVE;
        if (__KERNEL->uart_handlers[port]->cb->on_write_complete)
            __KERNEL->uart_handlers[port]->cb->on_write_complete(__KERNEL->uart_handlers[port]->param);

        //no more data? wait for completion and turn TX off
        while (__KERNEL->uart_handlers[port]->write_size == 0)
        {
            while ((UART_REGS[port]->SR & USART_SR_TC) == 0) {}
//            CRITICAL_ENTER_AGAIN;
            if ((UART_REGS[port]->SR & USART_SR_TC) && __KERNEL->uart_handlers[port]->write_size == 0)
                UART_REGS[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TXEIE);
//            CRITICAL_LEAVE;
        }
    }
    CRITICAL_LEAVE;
}
*/
void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
    int i;
    CRITICAL_ENTER;
    UART_REGS[(UART_PORT)param]->CR1 |= USART_CR1_TE;
    for(i = 0; i < size; ++i)
    {
        while ((UART_REGS[(UART_PORT)param]->SR & USART_SR_TXE) == 0) {}
        UART_REGS[(UART_PORT)param]->DR = buf[i];
    }
    //transmitter will be disabled int next IRQ TC
    CRITICAL_LEAVE;
}

/*
void uart_write(UART_PORT port, char* buf, int size)
{
    __KERNEL->uart_handlers[port]->write_buf = buf;
    __KERNEL->uart_handlers[port]->write_size = size;

    UART_REGS[port]->CR1 &= ~USART_CR1_TCIE;
    UART_REGS[port]->CR1 |= (USART_CR1_TE | USART_CR1_TXEIE);

}
*/

bool uart_set_baudrate(UART* uart, const UART_BAUD* config)
{
    unsigned int clock;
    HANDLE power = sys_get_object(SYS_OBJECT_POWER);
    if (power == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return false;
    }
    UART_REGS[uart->port]->CR1 &= ~USART_CR1_UE;

    if (config->data_bits == 8 && config->parity != 'N')
        UART_REGS[uart->port]->CR1 |= USART_CR1_M;
    else
        UART_REGS[uart->port]->CR1 &= ~USART_CR1_M;

    if (config->parity != 'N')
    {
        UART_REGS[uart->port]->CR1 |= USART_CR1_PCE;
        if (config->parity == 'O')
            UART_REGS[uart->port]->CR1 |= USART_CR1_PS;
        else
            UART_REGS[uart->port]->CR1 &= ~USART_CR1_PS;
    }
    else
        UART_REGS[uart->port]->CR1 &= ~USART_CR1_PCE;

    UART_REGS[uart->port]->CR2 = (config->stop_bits == 1 ? 0 : 2) << 12;
    UART_REGS[uart->port]->CR3 = 0;

    if (uart->port == UART_1 || uart->port == UART_6)
        clock = get(power, IPC_GET_CLOCK, STM32_CLOCK_APB2, 0, 0);
    else
        clock = get(power, IPC_GET_CLOCK, STM32_CLOCK_APB1, 0, 0);
    if (clock == 0)
        return false;
    unsigned int mantissa, fraction;
    mantissa = (25 * clock) / (4 * (config->baud));
    fraction = ((mantissa % 100) * 8 + 25)  / 50;
    mantissa = mantissa / 100;
    UART_REGS[uart->port]->BRR = (mantissa << 4) | fraction;

    UART_REGS[uart->port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
    UART_REGS[uart->port]->CR3 |= USART_CR3_EIE;

    UART_REGS[uart->port]->CR1 |= USART_CR1_TE;
    return true;
}

UART* uart_enable(UART_PORT port, UART_ENABLE* ue)
{
    UART* uart = NULL;
    HANDLE gpio = sys_get_object(SYS_OBJECT_GPIO);
    if (gpio == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return NULL;
    }
    if (port >= UARTS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return NULL;
    }
    uart = (UART*)malloc(sizeof(UART));
    if (uart == NULL)
    {
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
        return NULL;
    }

    uart->port = port;
    uart->tx_pin = ue->tx;
    uart->rx_pin = ue->rx;
    uart->error = ERROR_OK;
    if (uart->tx_pin == PIN_DEFAULT)
        uart->tx_pin = UART_TX_PINS[uart->port];
    if (uart->rx_pin == PIN_DEFAULT)
        uart->rx_pin = UART_RX_PINS[uart->port];

    //setup pins
#if defined(STM32F1)
    //turn on remapping
    if (((uart->tx_pin != UART_TX_PINS[uart->port]) && (uart->tx_pin != PIN_UNUSED)) ||
        ((uart->rx_pin != UART_RX_PINS[uart->port]) && (uart->rx_pin != PIN_UNUSED)))
    {
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        switch (uart->tx_pin)
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
            free(uart);
            return NULL;
        }
    }
    if (uart->tx_pin != PIN_UNUSED)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->tx_pin, GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    if (uart->rx_pin != PIN_UNUSED)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->rx_pin, GPIO_MODE_INPUT_FLOAT, false);
#elif defined(STM32F2) || defined(STM32F4)
    if (uart->tx_pin != PIN_UNUSED)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->tx_pin, GPIO_MODE_AF | GPIO_OT_PUSH_PULL |  GPIO_SPEED_HIGH, uart->port < UART_4 ? AF7 : AF8);
    if (uart->rx_pin != PIN_UNUSED)
        ack(gpio, IPC_ENABLE_PIN_SYSTEM, uart->rx_pin, , GPIO_MODE_AF | GPIO_SPEED_HIGH, uart->port < UART_4 ? AF7 : AF8);
#endif

    //power up
    if (uart->port == UART_1 || uart->port == UART_6)
        RCC->APB2ENR |= 1 << UART_POWER_PINS[uart->port];
    else
        RCC->APB1ENR |= 1 << UART_POWER_PINS[uart->port];

    //enable core
    UART_REGS[uart->port]->CR1 |= USART_CR1_UE;

    uart_set_baudrate(uart, &ue->baud);
    //enable interrupts
//            NVIC_EnableIRQ(UART_VECTORS[uart->port]);
//            NVIC_SetPriority(UART_VECTORS[uart->port], 15);
//            register_irq();
    return uart;
}

static inline bool uart_disable(UART* uart)
{
    HANDLE gpio = sys_get_object(SYS_OBJECT_GPIO);
    if (gpio == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return false;
    }
    //disable interrupts
//        NVIC_DisableIRQ(UART_VECTORS[port]);

    //disable core
    UART_REGS[uart->port]->CR1 &= ~USART_CR1_UE;
    //power down
    if (uart->port == UART_1 || uart->port == UART_6)
        RCC->APB2ENR &= ~(1 << UART_POWER_PINS[uart->port]);
    else
        RCC->APB1ENR &= ~(1 << UART_POWER_PINS[uart->port]);

    //disable pins
    if (uart->tx_pin != PIN_UNUSED)
        ack(gpio, IPC_DISABLE_PIN, uart->tx_pin, 0, 0);
    if (uart->rx_pin != PIN_UNUSED)
        ack(gpio, IPC_DISABLE_PIN, uart->rx_pin, 0, 0);

#if defined(STM32F1)
    //turn off remapping
    if (((uart->tx_pin != UART_TX_PINS[uart->port]) && (uart->tx_pin != PIN_UNUSED)) ||
        ((uart->rx_pin != UART_RX_PINS[uart->port]) && (uart->rx_pin != PIN_UNUSED)))
    {
        switch (uart->tx_pin)
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
    free(uart);
    return true;
}

static inline void stm32_uart_loop(UART** uarts)
{
    IPC ipc;
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case SYS_SET_STDOUT:
            __HEAP->stdout = (STDOUT)ipc.param1;
            __HEAP->stdout_param = (void*)ipc.param2;
            ipc_post(&ipc);
            break;
        case IPC_UART_ENABLE:
            //TODO need direct IO
            break;
        case IPC_UART_DISABLE:
            if (ipc.param1 < UARTS_COUNT)
            {
                if (uart_disable(uarts[ipc.param1]))
                {
                    uarts[ipc.param1] = NULL;
                    ipc_post(&ipc);
                }
                else
                    ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            }
            else
                ipc_post_error(ipc.process, get_last_error());
            break;
        case IPC_UART_SET_BAUDRATE:
            //TODO need direct IO
            break;
        case IPC_UART_GET_BAUDRATE:
            //TODO need direct IO
            break;
        case IPC_UART_FLUSH:
            //TODO need stream
            break;
        case IPC_UART_GET_TX_STREAM:
            //TODO need stream
            break;
        case IPC_UART_GET_RX_STREAM:
            //TODO need stream
            break;
        case IPC_UART_GET_LAST_ERROR:
            if (ipc.param1 < UARTS_COUNT)
            {
                ipc.param1 = uarts[ipc.param1]->error;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, get_last_error());
            break;
        case IPC_UART_CLEAR_ERROR:
            if (ipc.param1 < UARTS_COUNT)
            {
                uarts[ipc.param1]->error = ERROR_OK;
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, get_last_error());
            break;
            break;
//        case LISTENER INFO
//        case ISR_WRITE_COMPLETE
//        case ISR_READ_COMPLETE
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

void stm32_uart()
{
    UART* uarts[UARTS_COUNT] = {0};
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_UART, 0, 0);

#if (UART_STDIO)
    UART_ENABLE ue;
    ue.tx = UART_STDIO_TX;
    ue.rx = UART_STDIO_RX;
    ue.baud.data_bits = UART_STDIO_DATA_BITS;
    ue.baud.parity = UART_STDIO_PARITY;
    ue.baud.stop_bits = UART_STDIO_STOP_BITS;
    ue.baud.baud = UART_STDIO_BAUD;
    uarts[UART_STDIO_PORT] = uart_enable(UART_STDIO_PORT, &ue);

    setup_dbg(uart_write_kernel, (void*)UART_STDIO_PORT);
    //set global stdout
    setup_stdout(uart_write_kernel, (void*)UART_STDIO_PORT);
    //say early processes, that STDOUT is setted up
    __HEAP->stdout = (STDOUT)uart_write_kernel;
    __HEAP->stdout_param = (void*)UART_STDIO_PORT;
    sys_ack(SYS_SET_STDOUT, (unsigned int)uart_write_kernel, (unsigned int)UART_STDIO_PORT, 0);
    //power
    ack(sys_get_object(SYS_OBJECT_POWER), SYS_SET_STDOUT, (unsigned int)uart_write_kernel, (unsigned int)UART_STDIO_PORT, 0);
    //gpio
    ack(sys_get_object(SYS_OBJECT_GPIO), SYS_SET_STDOUT, (unsigned int)uart_write_kernel, (unsigned int)UART_STDIO_PORT, 0);
    //TODO timer
#endif //UART_STDIO

    stm32_uart_loop(uarts);
}
