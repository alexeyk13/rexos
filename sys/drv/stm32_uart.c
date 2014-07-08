/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_uart.h"
#include "stm32_power.h"
#include "stm32_gpio.h"
#include "../sys_call.h"
#include "arch.h"
#include "hw_config.h"
#include "gpio.h"
#include "dbg.h"
#include "error.h"
#include "../../../kernel/kernel.h"
#include "../../../userspace/lib/stdlib.h"

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


typedef USART_TypeDef* USART_TypeDef_P;

#if defined(STM32F1)
const USART_TypeDef_P USART[]                            = {USART1, USART2, USART3, UART4, UART5};
const GPIO_CLASS UART_TX_PINS[]                        = {USART1_TX_PIN, USART2_TX_PIN, USART3_TX_PIN, UART4_TX_PIN, GPIO_C12};
const GPIO_CLASS UART_RX_PINS[]                        = {USART1_RX_PIN, USART2_RX_PIN, USART3_RX_PIN, UART4_RX_PIN, GPIO_D2};
const uint32_t RCC_UART[] =                            {RCC_APB2ENR_USART1EN, RCC_APB1ENR_USART2EN, RCC_APB1ENR_USART3EN, RCC_APB1ENR_UART4EN, RCC_APB1ENR_UART5EN};
const IRQn_Type UART_IRQ_VECTORS[] =                {USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn};
#elif defined(STM32F2)
const USART_TypeDef_P USART[]                            = {USART1, USART2, USART3, UART4, UART5, USART6};
const GPIO_CLASS UART_TX_PINS[]                        = {USART1_TX_PIN, USART2_TX_PIN, USART3_TX_PIN, UART4_TX_PIN, GPIO_C12, USART6_TX_PIN};
const GPIO_CLASS UART_RX_PINS[]                        = {USART1_RX_PIN, USART2_RX_PIN, USART3_RX_PIN, UART4_RX_PIN, GPIO_D2, USART6_RX_PIN};
const uint32_t RCC_UART[] =                            {RCC_APB2ENR_USART1EN, RCC_APB1ENR_USART2EN, RCC_APB1ENR_USART3EN, RCC_APB1ENR_UART4EN, RCC_APB1ENR_UART5EN, RCC_APB2ENR_USART6EN};
const IRQn_Type UART_IRQ_VECTORS[] =                {USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn, USART6_IRQn};
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

void USART1_IRQHandler(void)
{
    uart_on_isr(UART_1);
}

void USART2_IRQHandler(void)
{
    uart_on_isr(UART_2);
}

void USART3_IRQHandler(void)
{
    uart_on_isr(UART_3);
}

void UART4_IRQHandler(void)
{
    uart_on_isr(UART_4);
}

void UART5_IRQHandler(void)
{
    uart_on_isr(UART_5);
}

void USART6_IRQHandler(void)
{
    uart_on_isr(UART_6);
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

extern void uart_enable(UART_CLASS port, UART_CB *cb, void *param, int priority)
{
    if (port < UARTS_COUNT)
    {
        UART_HW* uart = (UART_HW*)malloc(sizeof(UART_HW));
        if (uart)
        {
            __KERNEL->uart_handlers[port] = uart;
            uart->cb = cb;
            uart->param = param;
            uart->read_buf = NULL;
            uart->write_buf = NULL;
            uart->read_size = 0;
            uart->write_size = 0;
            //setup pins
#if defined(STM32F1)
            if ((USART_TX_DISABLE_MASK & (1 << port)) == 0)
                gpio_enable_afio(UART_TX_PINS[port], AFIO_MODE_PUSH_PULL);
            if ((USART_RX_DISABLE_MASK & (1 << port)) == 0)
                gpio_enable_pin(UART_RX_PINS[port], PIN_MODE_IN);
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
                gpio_enable_afio(UART_TX_PINS[port], port < UART_4 ? AFIO_MODE_USART1_2_3 : AFIO_MODE_UART_4_5_USART_6);
            if ((USART_RX_DISABLE_MASK & (1 << port)) == 0)
                gpio_enable_afio(UART_RX_PINS[port], port < UART_4 ? AFIO_MODE_USART1_2_3 : AFIO_MODE_UART_4_5_USART_6);
#endif

            //power up
            if (port == UART_1 || port == UART_6)
                RCC->APB2ENR |= RCC_UART[port];
            else
                RCC->APB1ENR |= RCC_UART[port];

            //enable interrupts
            NVIC_EnableIRQ(UART_IRQ_VECTORS[port]);
            NVIC_SetPriority(UART_IRQ_VECTORS[port], priority);

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
        NVIC_DisableIRQ(UART_IRQ_VECTORS[port]);

        //disable core
        USART[port]->CR1 &= ~USART_CR1_UE;
        //power down
        if (port == UART_1 || port == UART_6)
            RCC->APB2ENR &= ~RCC_UART[port];
        else
            RCC->APB1ENR &= ~RCC_UART[port];

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

/*        ipc.cmd = SYS_GET_POWER;
        if (!sys_call(&ipc))
            return;
        ipc.process = ipc.param1;
        ipc.cmd = IPC_GET_CLOCK;
        if (port == UART_1 || port == UART_6)
            ipc.param1 = STM32_CLOCK_APB2;
        else
            ipc.param1 = STM32_CLOCK_APB1;
        if (!call(&ipc))
            return;*/
        ipc.param1 = 35000000;
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
    uart_enable(UART_2, (P_UART_CB)NULL, NULL, 2);
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
