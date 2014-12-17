/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_uart.h"
#include "lpc_gpio.h"
#include "lpc_power.h"
#include "../../../userspace/irq.h"
#include "../../../userspace/stream.h"
#include "../../../userspace/direct.h"
#include "../../../userspace/stdlib.h"
#if (SYS_INFO)
#include "../../../userspace/stdio.h"
#endif

#if (MONOLITH_UART)
#include "lpc_core_private.h"


#define get_system_clock        lpc_power_get_system_clock_inside
#define ack_gpio                lpc_gpio_request_inside

#else

#define get_system_clock        lpc_power_get_system_clock_outside
#define ack_gpio                lpc_core_request_outside

void lpc_uart();

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
};

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
    IPC ipc;
    unsigned int lsr;
    SHARED_UART_DRV* drv = (SHARED_UART_DRV*)param;

    switch (LPC_USART->IIR & USART_IIR_INTID_MASK)
    {
    case USART_IIR_INTID_RLS:
        //decode error, if any
        lsr = LPC_USART->LSR;
        if (lsr & USART_LSR_OE)
            drv->uart.uarts[UART_0]->error = ERROR_OVERFLOW;
        if (lsr & USART_LSR_PE)
            drv->uart.uarts[UART_0]->error = ERROR_UART_PARITY;
        if (lsr & USART_LSR_FE)
            drv->uart.uarts[UART_0]->error = ERROR_UART_FRAME;
        if (lsr & USART_LSR_BI)
            drv->uart.uarts[UART_0]->error = ERROR_UART_BREAK;
        break;
    case USART_IIR_INTID_RDA:
        //receive data
        ipc.param1 = LPC_USART->RBR;
        ipc.process = process_iget_current();
        ipc.cmd = IPC_UART_ISR_RX;
        ipc.param3 = HAL_HANDLE(HAL_UART, UART_0);
        ipc_ipost(&ipc);
        break;
    case USART_IIR_INTID_THRE:
        //transmit more
        if ((LPC_USART->IER & USART_IER_THRINTEN) && drv->uart.uarts[UART_0]->tx_chunk_size)
        {
            while ((LPC_USART->LSR & USART_LSR_THRE) && drv->uart.uarts[UART_0]->tx_chunk_pos < drv->uart.uarts[UART_0]->tx_chunk_size)
                LPC_USART->THR = drv->uart.uarts[UART_0]->tx_buf[drv->uart.uarts[UART_0]->tx_chunk_pos++];
            //no more
            if (drv->uart.uarts[UART_0]->tx_chunk_pos >= drv->uart.uarts[UART_0]->tx_chunk_size)
            {
                drv->uart.uarts[UART_0]->tx_chunk_pos = drv->uart.uarts[UART_0]->tx_chunk_size = 0;
                ipc.process = process_iget_current();
                ipc.cmd = IPC_UART_ISR_TX;
                ipc.param2 = 0;
                ipc.param3 = HAL_HANDLE(HAL_UART, UART_0);
                ipc_ipost(&ipc);
                LPC_USART->IER &= ~USART_IER_THRINTEN;
            }
        }
        //transmission completed and no more data. Mask interrupt
        else
            LPC_USART->IER &= ~USART_IER_THRINTEN;
        break;
    }
}

#ifdef LPC11U6x
void lpc_uart4_on_isr(int vector, void* param)
{
    IPC ipc;
    unsigned int lsr;
    //find port by vector
    UART_PORT port;
    SHARED_UART_DRV* drv = (SHARED_UART_DRV*)param;
    if (vector == __UART_VECTORS[1])
        port = UART_1;
    else
        port = UART_3;
    for (i = 0; i < 2; ++i)
    {
        port = (vector == __UART_VECTORS[1] ? UART_1 : UART_2) + i * 2;
        if (drv->uart.uarts[port] == NULL)
            continue;

        //error condition
        if (__USART_REGS[port]->STAT & (USART4_STAT_OVERRUNINT | USART4_STAT_FRAMERRINT | USART4_STAT_PARITTERRINT | USART4_STAT_RXNOISEINT | USART4_STAT_DELTARXBRKINT))
        {
            if (__USART_REGS[port]->STAT & USART4_STAT_OVERRUNINT)
                drv->uart.uarts[port]->error = ERROR_OVERFLOW;
            if (__USART_REGS[port]->STAT & USART4_STAT_FRAMERRINT)
                drv->uart.uarts[port]->error = ERROR_UART_FRAME;
            if (__USART_REGS[port]->STAT & USART4_STAT_PARITTERRINT)
                drv->uart.uarts[port]->error = ERROR_UART_PARITY;
            if (__USART_REGS[port]->STAT & USART4_STAT_RXNOISEINT)
                drv->uart.uarts[port]->error = ERROR_UART_NOISE;
            if (__USART_REGS[port]->STAT & USART4_STAT_DELTARXBRKINT)
                drv->uart.uarts[port]->error = ERROR_UART_BREAK;
            __USART_REGS[port]->STAT = USART4_STAT_OVERRUNINT | USART4_STAT_FRAMERRINT | USART4_STAT_PARITTERRINT | USART4_STAT_RXNOISEINT | USART4_STAT_DELTARXBRKINT;
        }
        //ready to rx
        if (__USART_REGS[port]->STAT & USART4_STAT_RXRDY)
        {
            ipc.param1 = __USART_REGS[port]->RXDAT;
            ipc.process = process_iget_current();
            ipc.cmd = IPC_UART_ISR_RX;
            ipc.param3 = HAL_HANDLE(HAL_UART, UART_0);
            ipc_ipost(&ipc);
        }
        //tx
        if (__USART_REGS[port]->STAT & USART4_STAT_TXRDY && drv->uart.uarts[port]->tx_chunk_size)
        {
            while ((__USART_REGS[port]->STAT & USART4_STAT_TXRDY) && drv->uart.uarts[port]->tx_chunk_pos < drv->uart.uarts[port]->tx_chunk_size)
                __USART_REGS[port]->TXDAT = drv->uart.uarts[port]->tx_buf[drv->uart.uarts[port]->tx_chunk_pos++];
            //no more
            if (drv->uart.uarts[port]->tx_chunk_pos >= drv->uart.uarts[port]->tx_chunk_size)
            {
                drv->uart.uarts[port]->tx_chunk_pos = drv->uart.uarts[port]->tx_chunk_size = 0;
                ipc.process = process_iget_current();
                ipc.cmd = IPC_UART_ISR_TX;
                ipc.param2 = 0;
                ipc.param3 = HAL_HANDLE(HAL_UART, UART_0);
                ipc_ipost(&ipc);
                LPC_USART->IER &= ~USART_IER_THRINTEN;
            }
        }
    }
}
#endif

void lpc_uart_set_baudrate_internal(SHARED_UART_DRV* drv, UART_PORT port, const BAUD* config)
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

void lpc_uart_set_baudrate(SHARED_UART_DRV* drv, UART_PORT port, HANDLE process)
{
    BAUD baud;
    if (direct_read(process, (void*)&baud, sizeof(BAUD)))
        lpc_uart_set_baudrate_internal(drv, port, &baud);
}

void lpc_uart_open_internal(SHARED_UART_DRV *drv, UART_PORT port, UART_ENABLE* ue)
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
    if (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)
        ack_gpio(drv, LPC_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->tx_pin, GPIO_MODE_OUT, AF_UART);

    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
        ack_gpio(drv, LPC_GPIO_ENABLE_PIN_SYSTEM, drv->uart.uarts[port]->rx_pin, GPIO_MODE_NOPULL, AF_UART);
    //power up
    LPC_SYSCON->SYSAHBCLKCTRL |= 1 << __UART_POWER_PINS[port];
    //remove reset state. Only for LPC11U6x
#ifdef LPC11U6x
    if (port > UART_0)
    {
        LPC_SYSCON->PRESETCTRL |= 1 << __UART_RESET_PINS[port - 1];
        if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
            __USART_REGS[port]->INTENCSET = USART4_INTENSET_RXRDYEN | USART4_INTENSET_OVERRUNEN | USART4_INTENSET_FRAMERREN
                                          | USART4_INTENSET_PARITTERREN | USART4_INTENSET_RXNOISEEN | USART4_INTENSET_DELTARXBRKEN;
    }
    else
#endif
    {
         //enable FIFO
         LPC_USART->FCR |= USART_FCR_FIFOEN | USART_FCR_TXFIFORES | USART_FCR_RXFIFORES;
    }

    lpc_uart_set_baudrate_internal(drv, port, &ue->baud);

    //enable interrupts
#ifdef LPC11U6x
    if (port == UART_1 || port == UART_3)
    {
        if (drv->uart.uart13++ == 0)
            irq_register(__UART_VECTORS[port], lpc_uart4_on_isr, (void*)drv);
    }
    else if (port == UART_2 || port == UART_4)
    {
        if (drv->uart.uart24++ == 0)
            irq_register(__UART_VECTORS[port], lpc_uart4_on_isr, (void*)drv);
    }
    else
#endif
    {
        irq_register(__UART_VECTORS[port], lpc_uart_on_isr, (void*)drv);
    }
    irq_register(__UART_VECTORS[port], lpc_uart_on_isr, (void*)drv);
    NVIC_EnableIRQ(__UART_VECTORS[port]);
    NVIC_SetPriority(__UART_VECTORS[port], 13);
}

void lpc_uart_open(SHARED_UART_DRV* drv, UART_PORT port, HANDLE process)
{
    UART_ENABLE ue;
    if (direct_read(process, (void*)&ue, sizeof(UART_ENABLE)))
        lpc_uart_open_internal(drv, port, &ue);
}

static inline void lpc_uart_close(SHARED_UART_DRV* drv, UART_PORT port)
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
    NVIC_DisableIRQ(__UART_VECTORS[port]);
#ifdef LPC11U6x
    if (port == UART_1 || port == UART_3)
    {
        if (--drv->uart.uart13 == 0)
            irq_unregister(__UART_VECTORS[port]);
    }
    else if (port == UART_2 || port == UART_4)
    {
        if (--drv->uart.uart24 == 0)
            irq_unregister(__UART_VECTORS[port]);
    }
    else
#endif
    {
        irq_unregister(__UART_VECTORS[port]);
    }

    //power down
#ifdef LPC11U6x
    //set reset state. Only for LPC11U6x
    if (port > UART_0)
        LPC_SYSCON->PRESETCTRL &= ~(1 << __UART_RESET_PINS[port - 1]);
    else
#endif
    {
         //disable FIFO
         LPC_USART->FCR &= ~USART_FCR_FIFOEN;
    }
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << __UART_POWER_PINS[port]);

    //disable pins
    if (drv->uart.uarts[port]->tx_pin != PIN_UNUSED)
        ack_gpio(drv, GPIO_DISABLE_PIN, drv->uart.uarts[port]->tx_pin, 0, 0);

    if (drv->uart.uarts[port]->rx_pin != PIN_UNUSED)
        ack_gpio(drv, GPIO_DISABLE_PIN, drv->uart.uarts[port]->rx_pin, 0, 0);

    free(drv->uart.uarts[port]);
    drv->uart.uarts[port] = NULL;
}

static inline void lpc_uart_flush(SHARED_UART_DRV* drv, UART_PORT port)
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
#ifdef LPC11U6x
    if (port > UART_0)
    {
        __USART_REGS[port]->INTENCLR = USART4_INTENSET_TXRDYEN;
    }
    else
#endif
    {
        LPC_USART->IER &= ~USART_IER_THRINTEN;
        LPC_USART->FCR |= USART_FCR_TXFIFORES | USART_FCR_RXFIFORES;
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

static inline HANDLE lpc_uart_get_tx_stream(SHARED_UART_DRV* drv, UART_PORT port)
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

static inline HANDLE lpc_uart_get_rx_stream(SHARED_UART_DRV* drv, UART_PORT port)
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

static inline uint16_t lpc_uart_get_last_error(SHARED_UART_DRV* drv, UART_PORT port)
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

static inline void lpc_uart_clear_error(SHARED_UART_DRV* drv, UART_PORT port)
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

static inline void lpc_uart_write(SHARED_UART_DRV* drv, UART_PORT port, unsigned int total)
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
#ifdef LPC11U6x
            if (port > UART_0)
            {
                __USART_REGS[port]->INTENSET = USART4_INTENSET_TXRDYEN;
            }
            else
#endif
            {
                LPC_USART->IER |= USART_IER_THRINTEN;
            }
        }
    }
    else
        stream_listen(drv->uart.uarts[port]->tx_stream, (void*)HAL_HANDLE(HAL_UART, port));

}

static inline void lpc_uart_read(SHARED_UART_DRV* drv, UART_PORT port, char c)
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

void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
    UART_PORT port = (UART_PORT)param;
    NVIC_DisableIRQ(__UART_VECTORS[port]);
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
    NVIC_EnableIRQ(__UART_VECTORS[port]);
}

#if (UART_STDIO)
#if (SYS_INFO)
//we can't use printf in uart driver, because this can halt driver loop
void printu(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(fmt, va, uart_write_kernel, (void*)UART_STDIO_PORT);
    va_end(va);
}
#endif //(SYS_INFO)

static inline void lpc_uart_open_stdio(SHARED_UART_DRV* drv)
{
    UART_ENABLE ue;
    ue.tx = UART_STDIO_TX;
    ue.rx = UART_STDIO_RX;
    ue.baud.data_bits = UART_STDIO_DATA_BITS;
    ue.baud.parity = UART_STDIO_PARITY;
    ue.baud.stop_bits = UART_STDIO_STOP_BITS;
    ue.baud.baud = UART_STDIO_BAUD;
    ue.stream_size = STDIO_STREAM_SIZE;
    lpc_uart_open_internal(drv, UART_STDIO_PORT, &ue);

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

void lpc_uart_init(SHARED_UART_DRV* drv)
{
    int i;
#ifdef LPC11U6x
    LPC_SYSCON->USART0CLKDIV = 1;
    LPC_SYSCON->FRGCLKDIV = 1;
    drv->uart.uart13 = drv->uart.uart24 = 0;
#else
    LPC_SYSCON->UARTCLKDIV = 1;
#endif
    for (i = 0; i < UARTS_COUNT; ++i)
        drv->uart.uarts[i] = NULL;
#if (UART_STDIO)
    lpc_uart_open_stdio(drv);
#endif //UART_STDIO
}

#if (SYS_INFO)
static inline void lpc_uart_info(SHARED_UART_DRV* drv)
{
    int i;
    printu("LPC uart driver info\n\r\n\r");
    printu("uarts count: %d\n\r", UARTS_COUNT);

    for (i = 0; i < UARTS_COUNT; ++i)
    {
        if (drv->uart.uarts[i])
            printu("UART_%d ", i);
    }
    printu("\n\r\n\r");
}
#endif //SYS_INFO

bool lpc_uart_request(SHARED_UART_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        lpc_uart_info(drv);
        need_post = true;
        break;
#endif
    case IPC_OPEN:
        lpc_uart_open(drv, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        lpc_uart_close(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_SET_BAUDRATE:
        lpc_uart_set_baudrate(drv, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case IPC_FLUSH:
        lpc_uart_flush(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_GET_TX_STREAM:
        ipc->param1 = lpc_uart_get_tx_stream(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_GET_RX_STREAM:
        ipc->param1 = lpc_uart_get_rx_stream(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param1 = lpc_uart_get_last_error(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_UART_CLEAR_ERROR:
        lpc_uart_clear_error(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_STREAM_WRITE:
    case IPC_UART_ISR_TX:
        lpc_uart_write(drv, HAL_ITEM(ipc->param3), ipc->param2);
        //message from kernel (or ISR), no response
        break;
    case IPC_UART_ISR_RX:
        lpc_uart_read(drv, HAL_ITEM(ipc->param3), ipc->param1);
        //message from ISR, no response
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

#if !(MONOLITH_UART)
void lpc_uart()
{
    SHARED_UART_DRV drv;
    IPC ipc;
    bool need_post;
    object_set_self(SYS_OBJ_UART);
    lpc_uart_init(&drv);
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
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
            need_post = lpc_uart_request(&drv, &ipc);
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
#endif
