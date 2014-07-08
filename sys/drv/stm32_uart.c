/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "stm32_gpio.h"
#include "../sys_call.h"
#include "error.h"
#include "../../kernel/kernel.h"
#include "../../userspace/lib/stdlib.h"

#if defined(STM32F1)
#include "hw_config_stm32f1.h"
#endif

void stm32_uart();

const REX __STM32_UART = {
    //name
    "STM32 uart",
    //size
    512,
    //priority - driver priority
    91,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_uart
};

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
#define UARTS_COUNT                                         2
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3};

#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL)
#define UARTS_COUNT                                         3
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11};

#elif defined(STM32F1)
#define UARTS_COUNT                                         5
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {14, 17, 18, 19, 20};
///static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12};
///static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2};

#elif defined(STM32F2) || defined(STM32F40_41xxx)
#define UARTS_COUNT                                         6
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7};

#elif defined(STM32F4)
#define UARTS_COUNT                                         8
static const unsigned int UART_VECTORS[UARTS_COUNT] =       {37, 38, 39, 52, 53, 71, 82, 83};
static const unsigned int UART_POWER_PINS[UARTS_COUNT] =    {4, 17, 18, 19, 20, 5, 30, 31};
static const PIN UART_TX_PINS[UARTS_COUNT] =                {A9, A2, B10, C10, C12, C6, F7, F1};
static const PIN UART_RX_PINS[UARTS_COUNT] =                {A10, A3, B11, C11, D2, C7, F6, E0};

#endif

#define USART_RX_DISABLE_MASK_DEF                (0)
#define USART_TX_DISABLE_MASK_DEF                (0)

typedef USART_TypeDef* USART_TypeDef_P;

#if defined(STM32F1)
const USART_TypeDef_P USART[]                            = {USART1, USART2, USART3, UART4, UART5};
const PIN UART_TX_PINS[]                        = {USART1_TX_PIN, USART2_TX_PIN, USART3_TX_PIN, UART4_TX_PIN, C12};
const PIN UART_RX_PINS[]                        = {USART1_RX_PIN, USART2_RX_PIN, USART3_RX_PIN, UART4_RX_PIN, D2};
#elif defined(STM32F2)
const USART_TypeDef_P USART[]                            = {USART1, USART2, USART3, UART4, UART5, USART6};
const PIN UART_TX_PINS[]                        = {USART1_TX_PIN, USART2_TX_PIN, USART3_TX_PIN, UART4_TX_PIN, C12, USART6_TX_PIN};
const PIN UART_RX_PINS[]                        = {USART1_RX_PIN, USART2_RX_PIN, USART3_RX_PIN, UART4_RX_PIN, D2, USART6_RX_PIN};
#endif



#define UART_ERROR_MASK                                    0xf

#define DISABLE_RECEIVER()                                USART[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE)
#define ENABLE_RECEIVER()                                USART[port]->CR1 |= (USART_CR1_RE | USART_CR1_RXNEIE)
#define ENABLE_TRANSMITTER()                            USART[port]->CR1 |= USART_CR1_TE
#define DISABLE_TRANSMITTER()                            USART[port]->CR1 &= ~(USART_CR1_TE)

const int USART_RX_DISABLE_MASK =                    USART_RX_DISABLE_MASK_DEF;
const int USART_TX_DISABLE_MASK =                    USART_TX_DISABLE_MASK_DEF;

void uart_on_isr(UART_CLASS port)
{
    if (__KERNEL->uart_handlers[port])
    {
        __KERNEL->uart_handlers[port]->isr_active = true;
        uint16_t sr = USART[port]->SR;

        //slave: transmit more
        if ((sr & USART_SR_TXE) && __KERNEL->uart_handlers[port]->write_size)
        {
            USART[port]->DR = __KERNEL->uart_handlers[port]->write_buf[0];
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
                USART[port]->CR1 &= ~USART_CR1_TXEIE;
                USART[port]->CR1 |= USART_CR1_TCIE;
            }
        }
        //transmission completed and no more data
        else if ((sr & USART_SR_TC) && __KERNEL->uart_handlers[port]->write_size == 0)
            USART[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
        //decode error, if any
        if ((sr & UART_ERROR_MASK))
        {
//            UART_ERROR error = UART_ERROR_OK;
//            if (sr & USART_SR_ORE)
//                error = UART_ERROR_OVERRUN;
//            else
//            {
                __REG_RC32(USART[port]->DR);
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
            __KERNEL->uart_handlers[port]->read_buf[0] = USART[port]->DR;
            __KERNEL->uart_handlers[port]->read_buf++;
            if (--__KERNEL->uart_handlers[port]->read_size == 0)
            {
                __KERNEL->uart_handlers[port]->read_buf = NULL;
//                if (__KERNEL->uart_handlers[port]->cb->on_read_complete)
//                    __KERNEL->uart_handlers[port]->cb->on_read_complete(__KERNEL->uart_handlers[port]->param);
            }
            //no more, disable receiver
            if (__KERNEL->uart_handlers[port]->read_size == 0)
                USART[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
        }
        else if (sr & USART_SR_RXNE)
        {
            __REG_RC32(USART[port]->DR);
            USART[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
        }
    }
}

void uart_read(UART_CLASS port, char* buf, int size)
{
    //must be handled be upper layer
    ASSERT(__KERNEL->uart_handlers[port]->read_size == 0);

    __KERNEL->uart_handlers[port]->read_buf = buf;
    __KERNEL->uart_handlers[port]->read_size = size;

    USART[port]->CR1 |= (USART_CR1_RE | USART_CR1_RXNEIE);
}

void uart_read_cancel(UART_CLASS port)
{
    __KERNEL->uart_handlers[port]->read_size = 0;
    __KERNEL->uart_handlers[port]->read_buf = NULL;
    USART[port]->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE);
}

void uart_write_wait(UART_CLASS port)
{
    CRITICAL_ENTER;
    __KERNEL->uart_handlers[port]->isr_active = false;
    while (__KERNEL->uart_handlers[port]->write_size != 0)
    {
        while ((USART[port]->SR & USART_SR_TXE) == 0) {}
//        CRITICAL_ENTER;
        //not handled in isr, do it manually
        if ((USART[port]->SR & USART_SR_TXE) && !__KERNEL->uart_handlers[port]->isr_active)
        {
            USART[port]->DR = __KERNEL->uart_handlers[port]->write_buf[0];
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
            while ((USART[port]->SR & USART_SR_TC) == 0) {}
//            CRITICAL_ENTER_AGAIN;
            if ((USART[port]->SR & USART_SR_TC) && __KERNEL->uart_handlers[port]->write_size == 0)
                USART[port]->CR1 &= ~(USART_CR1_TE | USART_CR1_TXEIE);
//            CRITICAL_LEAVE;
        }
    }
    CRITICAL_LEAVE;
}

void uart_write_svc(const char *const buf, unsigned int size, void* param)
{
    int i;
    CRITICAL_ENTER;
    USART[(UART_CLASS)param]->CR1 |= USART_CR1_TE;
    for(i = 0; i < size; ++i)
    {
        while ((USART[(UART_CLASS)param]->SR & USART_SR_TXE) == 0) {}
        USART[(UART_CLASS)param]->DR = buf[i];
    }
    //transmitter will be disabled int next IRQ TC
    CRITICAL_LEAVE;
}

void uart_write(UART_CLASS port, char* buf, int size)
{
    __KERNEL->uart_handlers[port]->write_buf = buf;
    __KERNEL->uart_handlers[port]->write_size = size;

    USART[port]->CR1 &= ~USART_CR1_TCIE;
    USART[port]->CR1 |= (USART_CR1_TE | USART_CR1_TXEIE);

}

extern void uart_enable(UART_CLASS port, int priority)
{
    if (port < UARTS_COUNT)
    {
        UART_HW* uart = (UART_HW*)malloc(sizeof(UART_HW));
        if (uart)
        {
            __KERNEL->uart_handlers[port] = uart;
            uart->read_buf = NULL;
            uart->write_buf = NULL;
            uart->read_size = 0;
            uart->write_size = 0;
            //setup pins
#if defined(STM32F1)
            if ((USART_TX_DISABLE_MASK & (1 << port)) == 0)
                gpio_enable_pin_system(UART_TX_PINS[port], GPIO_MODE_OUTPUT_AF_PUSH_PULL_10MHZ, false);
            if ((USART_RX_DISABLE_MASK & (1 << port)) == 0)
                gpio_enable_pin_system(UART_RX_PINS[port], GPIO_MODE_INPUT_FLOAT, false);
//#if (USART_REMAP_MASK)
//            if ((1 << port) & USART_REMAP_MASK)
            {
                afio_remap();
                //TODO: let it work for UART1/3
                AFIO->MAPR |= (1 << 3);
            }
//#endif //USART_REMAP_MASK
#elif defined(STM32F2)
            if ((USART_TX_DISABLE_MASK & (1 << port)) == 0)
            {
                gpio_enable_pin_power(UART_TX_PINS[port]);
                gpio_enable_afio(UART_TX_PINS[port], port < UART_4 ? AFIO_MODE_USART1_2_3 : AFIO_MODE_UART_4_5_USART_6);
            }
            if ((USART_RX_DISABLE_MASK & (1 << port)) == 0)
            {
                gpio_enable_pin_power(UART_RX_PINS[port]);
                gpio_enable_afio(UART_RX_PINS[port], port < UART_4 ? AFIO_MODE_USART1_2_3 : AFIO_MODE_UART_4_5_USART_6);
            }
#endif

            //power up
            if (port == UART_1 || port == UART_6)
                RCC->APB2ENR |= 1 << UART_POWER_PINS[port];
            else
                RCC->APB1ENR |= 1 << UART_POWER_PINS[port];

            //enable interrupts
            NVIC_EnableIRQ(UART_VECTORS[port]);
            NVIC_SetPriority(UART_VECTORS[port], priority);

            USART[port]->CR1 |= USART_CR1_UE;
        }
        else
            error(ERROR_OUT_OF_SYSTEM_MEMORY);
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void uart_disable(UART_CLASS port)
{
    if (port < UARTS_COUNT)
    {
        //disable interrupts
        NVIC_DisableIRQ(UART_VECTORS[port]);

        //disable core
        USART[port]->CR1 &= ~USART_CR1_UE;
        //power down
        if (port == UART_1 || port == UART_6)
            RCC->APB2ENR &= ~(1 << UART_POWER_PINS[port]);
        else
            RCC->APB1ENR &= ~(1 << UART_POWER_PINS[port]);

        //disable pins
        if ((USART_TX_DISABLE_MASK & (1 << port)) == 0)
            gpio_disable_pin(UART_TX_PINS[port]);
        if ((USART_RX_DISABLE_MASK & (1 << port)) == 0)
            gpio_disable_pin(UART_RX_PINS[port]);

#if (USART_REMAP_MASK)
        if ((1 << port) & USART_REMAP_MASK)
            afio_unmap();
#endif //USART_REMAP_MASK

        free(__KERNEL->uart_handlers[port]);
        __KERNEL->uart_handlers[port] = NULL;
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void uart_set_baudrate(UART_CLASS port, const UART_BAUD* config)
{
    IPC ipc;
    if (port < UARTS_COUNT)
    {
        USART[port]->CR1 &= ~USART_CR1_UE;

        if (config->data_bits == 8 && config->parity != 'N')
            USART[port]->CR1 |= USART_CR1_M;
        else
            USART[port]->CR1 &= ~USART_CR1_M;

        if (config->parity != 'N')
        {
            USART[port]->CR1 |= USART_CR1_PCE;
            if (config->parity == 'O')
                USART[port]->CR1 |= USART_CR1_PS;
            else
                USART[port]->CR1 &= ~USART_CR1_PS;
        }
        else
            USART[port]->CR1 &= ~USART_CR1_PCE;

        USART[port]->CR2 = (config->stop_bits == 1 ? 0 : 2) << 12;
        USART[port]->CR3 = 0;

        ipc.cmd = SYS_GET_POWER;
        if (!sys_call(&ipc))
            return;
        ipc.process = ipc.param1;
        ipc.cmd = IPC_GET_CLOCK;
        if (port == UART_1 || port == UART_6)
            ipc.param1 = STM32_CLOCK_APB2;
        else
            ipc.param1 = STM32_CLOCK_APB1;
        if (!call(&ipc))
            return;
        unsigned int mantissa, fraction;
        mantissa = (25 * ipc.param1) / (4 * (config->baud));
        fraction = ((mantissa % 100) * 8 + 25)  / 50;
        mantissa = mantissa / 100;
        USART[port]->BRR = (mantissa << 4) | fraction;

        USART[port]->CR1 |= USART_CR1_UE | USART_CR1_PEIE;
        USART[port]->CR3 |= USART_CR3_EIE;

        USART[port]->CR1 |= USART_CR1_TE;
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void stm32_uart()
{
    IPC ipc;
    uart_enable(UART_2, 2);
    UART_BAUD baud;
    baud.data_bits = 8;
    baud.parity = 'N';
    baud.stop_bits = 1;
    baud.baud = 115200;
    uart_set_baudrate(UART_2, &baud);

    setup_dbg(uart_write_svc, (void*)UART_2);
    //refactor me later
    setup_stdout(uart_write_svc, (void*)UART_2);

    ipc.cmd = SYS_SET_UART;
    ipc.param1 = __HEAP->handle;
    sys_call(&ipc);
    for (;;)
    {
        sleep_ms(0);
    }
}
