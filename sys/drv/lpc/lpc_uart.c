/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_uart.h"
#include "lpc_gpio.h"
#include "lpc_power.h"
#include "../../../userspace/irq.h"

#if (MONOLITH_UART)
#include "lpc_core_private.h"


#define get_system_clock        lpc_power_get_system_clock_inside
#define ack_gpio                lpc_gpio_request_inside

#else

#define get_system_clock        lpc_power_get_system_clock_outside
#define ack_gpio                lpc_core_request_outside

/*void lpc_uart();

const REX __LPC_UART = {
    //name
    "LPC uart",
    //size
    LPC_UART_STACK_SIZE,
    //priority - driver priority. Setting priority lower than other drivers can cause IPC overflow on SYS_INFO
    89,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    LPC_DRIVERS_IPC_COUNT,
    //function
    lpc_uart
};*/

#endif

#ifdef LPC11U6x
static const uint8_t __UART_RESET_PINS[] =          {SYSCON_PRESETCTRL_USART1_RST_N_POS, SYSCON_PRESETCTRL_USART2_RST_N_POS, SYSCON_PRESETCTRL_USART3_RST_N_POS,
                                                     SYSCON_PRESETCTRL_USART4_RST_N_POS};
static const uint8_t __UART_POWER_PINS[] =          {SYSCON_SYSAHBCLKCTRL_USART0_POS, SYSCON_SYSAHBCLKCTRL_USART1_POS, SYSCON_SYSAHBCLKCTRL_USART2_POS,
                                                     SYSCON_SYSAHBCLKCTRL_USART3_4_POS, SYSCON_SYSAHBCLKCTRL_USART3_4_POS};
static const uint8_t __UART_VECTORS[] =             {21, 11, 12, 12, 11};
#define LPC_USART                                   LPC_USART0

typedef LPC_USART4_Type* LPC_USART4_Type_P;

static const LPC_USART4_Type_P __USART_REGS[] =     {LPC_USART1, LPC_USART2, LPC_USART3, LPC_USART4};
#else
static const uint8_t __UART_POWER_PINS[] =          {SYSCON_SYSAHBCLKCTRL_USART0_POS};
static const uint8_t __UART_VECTORS[] =             {21};
#endif

void lpc_uart_on_isr(int vector, void* param)
{
    printk("isr\n\r");
}

void lpc_uart_set_baudrate_internal(UART_DRV* drv, UART_PORT port, const BAUD* config)
{
    if (port >= UARTS_COUNT)
    {
//        error(ERROR_INVALID_PARAMS);
        return;
    }
/*    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }*/
    unsigned int divider = get_system_clock(drv) / 16 / config->baud;
#ifdef LPC11U6x
    if (port > UART_0)
    {
        __USART_REGS[port - 1]->CFG = ((config->data_bits - 7) << USART4_CFG_DATALEN_POS) | ((config->stop_bits - 1) << USART4_CFG_STOPLEN_POS);
        switch (config->parity)
        {
        case 'O':
            __USART_REGS[port - 1]->CFG |= USART4_CFG_PARITYSEL_ODD;
            break;
        case 'E':
            __USART_REGS[port - 1]->CFG |= USART4_CFG_PARITYSEL_EVEN;
            break;
        }
        __USART_REGS[port - 1]->BRG = divider & 0xffff;
        __USART_REGS[port - 1]->CFG |= USART4_CFG_ENABLE
    }
    else
#endif
    {
        LPC_USART->LCR = ((config->data_bits - 5) << USART_LCR_WLS_POS) | ((config->stop_bits - 1) << USART_LCR_SBS_POS) | USART_LCR_DLAB;
        switch (config->parity)
        {
        case 'O':
            LPC_USART->LCR |= USART_LCR_PE | USART_LCR_PS_ODD;
            break;
        case 'E':
            LPC_USART->LCR |= USART_LCR_PE | USART_LCR_PS_EVEN;
            break;
        }
        LPC_USART->DLM = (divider >> 8) & 0xff;
        LPC_USART->DLL = divider & 0xff;
        LPC_USART->LCR &= ~USART_LCR_DLAB;
    }
}

void lpc_uart_open_internal(UART_DRV* drv, UART_PORT port, UART_ENABLE* ue)
{
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
/*    if (drv->uart.uarts[port] != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    drv->uart.uarts[port] = malloc(sizeof(UART));*/
/*    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }*/
    //TODO: add "uart."
    drv->uarts[port].tx_pin = ue->tx;
    drv->uarts[port].rx_pin = ue->rx;
/*    drv->uart.uarts[port]->error = ERROR_OK;
    drv->uart.uarts[port]->tx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->rx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->tx_total = 0;
    drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
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
    }*/

    //setup pins
    //TODO: add "uart."
    if (drv->uarts[port].tx_pin != PIN_UNUSED)
        ack_gpio(drv, LPC_GPIO_ENABLE_PIN_SYSTEM, drv->uarts[port].tx_pin, GPIO_MODE_OUT, AF_UART);

    //TODO: add "uart."
    if (drv->uarts[port].rx_pin != PIN_UNUSED)
        ack_gpio(drv, LPC_GPIO_ENABLE_PIN_SYSTEM, drv->uarts[port].rx_pin, GPIO_MODE_NOPULL, AF_UART);
    //power up
    LPC_SYSCON->SYSAHBCLKCTRL |= 1 << __UART_POWER_PINS[port];
    //remove reset state. Only for LPC11U6x
#ifdef LPC11U6x
    if (port > UART_0)
        LPC_SYSCON->PRESETCTRL |= 1 << __UART_RESET_PINS[port - 1];
    else
#endif
    {
         //enable FIFO
         LPC_USART->FCR |= USART_FCR_FIFOEN;
    }

    lpc_uart_set_baudrate_internal(drv, port, &ue->baud);

    //enable interrupts
    irq_register(__UART_VECTORS[port], lpc_uart_on_isr, (void*)drv);
    NVIC_EnableIRQ(__UART_VECTORS[port]);
    NVIC_SetPriority(__UART_VECTORS[port], 13);
}

void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
///    UART_PORT port = (UART_PORT)param;
///    NVIC_DisableIRQ(__UART_VECTORS[port]);
    int i;
    for(i = 0; i < size; ++i)
    {
#ifdef LPC11U6x
        if (port > UART_0)
        {
            while ((__USART_REGS[port - 1]->STAT & USART4_STAT_TXRDY) == 0) {}
            __USART_REGS[port - 1]->TXDAT = buf[i];
        }
        else
#endif
        {
            while ((LPC_USART->LSR & USART_LSR_THRE) == 0) {}
            LPC_USART->THR = buf[i];
        }
    }
///    NVIC_EnableIRQ(__UART_VECTORS[port]);
}

void lpc_uart_init(UART_DRV *drv)
{
#ifdef LPC11U6x
    LPC_SYSCON->USART0CLKDIV = 1;
    LPC_SYSCON->FRGCLKDIV = 1;
#else
    LPC_SYSCON->UARTCLKDIV = 1;
#endif
//    int i;
//    for (i = 0; i < UARTS_COUNT; +i)
//          drv[i] = NULL;
}
