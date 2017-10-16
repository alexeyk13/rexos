/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_uart.h"
#include "ti_exo_private.h"
#include "ti_power.h"
#include "../kirq.h"
#include "../kipc.h"
#include "../kstream.h"
#include "../kernel.h"
#include "../kerror.h"
#include "../../userspace/uart.h"
#include "../../userspace/stream.h"
#include "../../userspace/ti/ti.h"

#define FIFO_MAX_DEPTH                                                          32

void ti_uart_on_isr(int vector, void* param)
{
    EXO* exo = param;
    char rx_buf[FIFO_MAX_DEPTH];
    unsigned int rx_size;

    while ((exo->uart.rx_handle != INVALID_HANDLE) && (UART0->FR & UART_FR_RXFE))
    {
        if (UART0->RSR & 0xf)
        {
            if (UART0->RSR & UART_RSR_OE)
                exo->uart.error = ERROR_OVERFLOW;
            if (UART0->RSR & UART_RSR_PE)
                exo->uart.error = ERROR_INVALID_PARITY;
            if (UART0->RSR & UART_RSR_FE)
                exo->uart.error = ERROR_INVALID_FRAME;
            UART0->ECR = 0;
        }

        UART0->ICR = UART_ICR_RXIC;
        for (rx_size = 0; (rx_size < FIFO_MAX_DEPTH) && ((UART0->FR & UART_FR_RXFE) == 0); ++rx_size)
            rx_buf[rx_size] = UART0->DR & 0xff;
        if (stream_iwrite_no_block(exo->uart.rx_handle, rx_buf, rx_size) < rx_size)
            exo->uart.error = ERROR_CHAR_LOSS;
    }

    if ((UART0->IMSC & UART_IMSC_TXIM))
    {
        if (exo->uart.tx_size == 0)
        {
            exo->uart.tx_total = exo->uart.tx_size = stream_iread_no_block(exo->uart.tx_handle, exo->uart.tx_buf, UART_BUF_SIZE);
            //nothing more, listen
            if (exo->uart.tx_size == 0)
            {
                UART0->IMSC &= ~UART_IMSC_TXIM;
                stream_ilisten(exo->uart.tx_stream, UART_0, HAL_UART);
                return;
            }
        }

        for(; ((UART0->FR & UART_FR_TXFF) == 0) && exo->uart.tx_size; --exo->uart.tx_size)
            UART0->DR = exo->uart.tx_buf[exo->uart.tx_total - exo->uart.tx_size];
    }
}

void ti_uart_init(EXO* exo)
{
    exo->uart.active = false;
}

static void ti_uart_destroy(EXO* exo)
{
    kstream_close(KERNEL_HANDLE, exo->uart.tx_handle);
    kstream_close(KERNEL_HANDLE, exo->uart.rx_handle);
    kstream_destroy(exo->uart.tx_stream);
    kstream_destroy(exo->uart.rx_stream);
    exo->uart.active = false;
}

static inline void ti_uart_enable_clock(EXO* exo)
{
    ti_power_domain_on(exo, POWER_DOMAIN_SERIAL);

    //gate clock for UART
    PRCM->UARTCLKGR = PRCM_UARTCLKGR_CLK_EN;
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}
}

static void ti_uart_disable_clock(EXO* exo)
{
    //gate clock for UART
    PRCM->UARTCLKGR &= ~PRCM_UARTCLKGR_CLK_EN;
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

    ti_power_domain_off(exo, POWER_DOMAIN_SERIAL);
}

static inline bool ti_uart_open_stream(EXO* exo, unsigned int mode)
{
    exo->uart.tx_stream = INVALID_HANDLE;
    exo->uart.tx_handle = INVALID_HANDLE;
    exo->uart.rx_stream = INVALID_HANDLE;
    exo->uart.rx_handle = INVALID_HANDLE;
    exo->uart.tx_size = 0;
    if (mode & UART_TX_STREAM)
    {
        exo->uart.tx_stream = kstream_create(UART_STREAM_SIZE);
        exo->uart.tx_handle = kstream_open(KERNEL_HANDLE, exo->uart.tx_stream);
        if (exo->uart.tx_handle == INVALID_HANDLE)
            return false;
        kstream_listen(KERNEL_HANDLE, exo->uart.tx_stream, UART_0, HAL_UART);
    }
    if (mode & UART_RX_STREAM)
    {
        exo->uart.rx_stream = kstream_create(UART_STREAM_SIZE);
        exo->uart.rx_handle = kstream_open(KERNEL_HANDLE, exo->uart.rx_stream);
        if (exo->uart.tx_handle == INVALID_HANDLE)
            return false;
    }
    if (mode & UART_RX_STREAM)
    {
        UART0->CTL |= UART_CTL_RXE;
        UART0->IMSC |= UART_IMSC_RXIM | UART_IMSC_OEIM | UART_IMSC_FEIM | UART_IMSC_PEIM;
    }
    if (mode & UART_TX_STREAM)
        UART0->CTL |= UART_CTL_TXE;
    return true;
}

static inline void ti_uart_open(EXO* exo, unsigned int mode)
{
    bool ok;

    exo->uart.error = ERROR_OK;

    ti_uart_enable_clock(exo);

    UART0->CTL = 0;

    switch (mode & UART_MODE)
    {
    case UART_MODE_STREAM:
        ok = ti_uart_open_stream(exo, mode);
        break;
    default:
        ok = false;
    }
    if (!ok)
    {
        ti_uart_disable_clock(exo);
        ti_uart_destroy(exo);
        return;
    }

    UART0->CTL |= UART_CTL_UARTEN;
    exo->uart.active = true;

    //enable interrupts
    NVIC_EnableIRQ(UART0_IRQn);
    NVIC_SetPriority(UART0_IRQn, 6);
    kirq_register(KERNEL_HANDLE, UART0_IRQn, ti_uart_on_isr, exo);
}

static void ti_uart_flush(EXO* exo)
{
    exo->uart.error = ERROR_OK;
    if (exo->uart.tx_stream != INVALID_HANDLE)
    {
        kstream_flush(exo->uart.tx_stream);
        kstream_listen(KERNEL_HANDLE, exo->uart.tx_stream, UART_0, HAL_UART);
        __disable_irq();
        exo->uart.tx_size = 0;
        __enable_irq();
    }
    if (exo->uart.rx_stream != INVALID_HANDLE)
        kstream_flush(exo->uart.rx_stream);
}

static inline void ti_uart_close(EXO* exo)
{
    //disable interrupts
    NVIC_DisableIRQ(UART0_IRQn);
    kirq_unregister(KERNEL_HANDLE, UART0_IRQn);
    ti_uart_flush(exo);

    //disable transmitter/receiver
    UART0->CTL = 0;

    ti_uart_disable_clock(exo);
    ti_uart_destroy(exo);
}

static inline void ti_uart_set_baudrate(IPC* ipc)
{
    unsigned int divider, fraction;
    BAUD baudrate;
    uart_decode_baudrate(ipc, &baudrate);

    UART0->CTL &= ~UART_CTL_UARTEN;

    divider =  (ti_power_get_core_clock() << 2) / baudrate.baud;
    fraction = divider & 0x3f;
    divider = (divider - fraction) / 64;

    UART0->IBRD = divider;
    UART0->FBRD = fraction;

    UART0->LCRH = (UART0->LCRH) & UART_LCRH_FEN;
    switch (baudrate.data_bits)
    {
    case 5:
    case 6:
    case 7:
        UART0->LCRH |= UART_LCRH_WLEN_VAL(baudrate.data_bits - 5);
        break;
    default:
        UART0->LCRH |= UART_LCRH_WLEN_8;
    }

    if (baudrate.stop_bits == 2)
        UART0->LCRH |= UART_LCRH_STP2;

    switch (baudrate.parity)
    {
    case 'E':
        UART0->LCRH |= UART_LCRH_PEN | UART_LCRH_EPS;
        break;
    case 'O':
        UART0->LCRH |= UART_LCRH_PEN;
        break;
    default:
        break;
    }

    UART0->CTL |= UART_CTL_UARTEN;
}

void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
    int i;
    NVIC_DisableIRQ(UART0_IRQn);
    for(i = 0; i < size; ++i)
    {
        while (UART0->FR & UART_FR_TXFF) {}
        UART0->DR = buf[i];
    }
    NVIC_EnableIRQ(UART0_IRQn);
}

static inline HANDLE ti_uart_get_tx_stream(EXO* exo)
{
    return exo->uart.tx_stream;
}

static inline HANDLE ti_uart_get_rx_stream(EXO* exo)
{
    return exo->uart.rx_stream;
}

static inline uint16_t ti_uart_get_last_kerror(EXO* exo)
{
    return exo->uart.error;
}

static inline void ti_uart_clear_kerror(EXO* exo)
{
    exo->uart.error = ERROR_OK;
}

static inline void ti_uart_setup_printk()
{
    kernel_setup_dbg(uart_write_kernel, NULL);
}

static inline void ti_uart_stream_write(EXO* exo)
{
    UART0->IMSC |= UART_IMSC_TXIM;
}

void ti_uart_request(EXO* exo, IPC* ipc)
{
    if (ipc->param1 > UART_0)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if (HAL_ITEM(ipc->cmd) == IPC_OPEN)
    {
        ti_uart_open(exo, ipc->param2);
        return;
    }
    if (!exo->uart.active)
    {
        kerror(ERROR_NOT_ACTIVE);
        return;
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_CLOSE:
        ti_uart_close(exo);
        break;
    case IPC_UART_SET_BAUDRATE:
        ti_uart_set_baudrate(ipc);
        break;
    case IPC_FLUSH:
        ti_uart_flush(exo);
        break;
    case IPC_GET_TX_STREAM:
        ipc->param2 = ti_uart_get_tx_stream(exo);
        break;
    case IPC_GET_RX_STREAM:
        ipc->param2 = ti_uart_get_rx_stream(exo);
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param2 = ti_uart_get_last_kerror(exo);
        break;
    case IPC_UART_CLEAR_ERROR:
        ti_uart_clear_kerror(exo);
        break;
    case IPC_UART_SETUP_PRINTK:
        ti_uart_setup_printk();
        break;
    case IPC_STREAM_WRITE:
        ti_uart_stream_write(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}
