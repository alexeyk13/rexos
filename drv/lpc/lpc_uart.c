/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_uart.h"
#include "lpc_power.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "../../userspace/irq.h"
#include "../../userspace/stream.h"
#include "../../userspace/stdlib.h"
#include "lpc_core_private.h"

typedef enum {
    IPC_UART_ISR_TX = IPC_UART_MAX,
    IPC_UART_ISR_RX
} LPC_UART_IPCS;

#if defined(LPC11U6x)
static const uint8_t __UART_RESET_PINS[] =                      {SYSCON_PRESETCTRL_USART1_RST_N_POS, SYSCON_PRESETCTRL_USART2_RST_N_POS, SYSCON_PRESETCTRL_USART3_RST_N_POS,
                                                                 SYSCON_PRESETCTRL_USART4_RST_N_POS};
static const uint8_t __UART_POWER_PINS[] =                      {SYSCON_SYSAHBCLKCTRL_USART0_POS, SYSCON_SYSAHBCLKCTRL_USART1_POS, SYSCON_SYSAHBCLKCTRL_USART2_POS,
                                                                 SYSCON_SYSAHBCLKCTRL_USART3_4_POS, SYSCON_SYSAHBCLKCTRL_USART3_4_POS};
static const uint8_t __UART_VECTORS[] =                         {21, 11, 12, 12, 11};
#define LPC_USART                                               LPC_USART0

typedef LPC_USART4_Type* LPC_USART4_Type_P;

static const LPC_USART4_Type_P __USART1_REGS[] =                {LPC_USART1, LPC_USART2, LPC_USART3, LPC_USART4};
typedef LPC_USART_Type* LPC_USART_Type_P
static const LPC_USART_Type_P __USART_REGS[UARTS_COUNT] =       {LPC_USART};
#elif defined(LPC11Uxx)
static const uint8_t __UART_POWER_PINS[] =                      {SYSCON_SYSAHBCLKCTRL_USART0_POS};
static const uint8_t __UART_VECTORS[] =                         {21};

typedef LPC_USART_Type* LPC_USART_Type_P;
static const LPC_USART_Type_P __USART_REGS[UARTS_COUNT] =       {LPC_USART};
#else //LPC18xx
static const uint8_t __UART_VECTORS[UARTS_COUNT] =              {24, 25, 26, 27};

typedef LPC_USARTn_Type* LPC_USARTn_Type_P;
static const LPC_USARTn_Type_P __USART_REGS[UARTS_COUNT] =      {LPC_USART0, (LPC_USARTn_Type_P)LPC_UART1, LPC_USART2, LPC_USART3};

#define LPC_CGU_UART0_CLOCK_BASE                                0x4005009c

#endif

static bool lpc_uart_rx_isr(CORE* drv, UART_PORT port, uint8_t c)
{
    bool res = true;
    UART* uart = drv->uart.uarts[port];
#if (UART_IO_MODE_SUPPORT)
    if (drv->uart.uarts[port]->io_mode)
    {
        timer_istop(uart->i.rx_timer);
        if (uart->i.rx_io)
        {
            ((uint8_t*)io_data(uart->i.rx_io))[uart->i.rx_io->data_size++] = c;
            //max size limit
            if (uart->i.rx_io->data_size >= uart->i.rx_max)
            {
                iio_complete(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, uart->i.rx_io);
                uart->i.rx_io = NULL;
                res = false;
            }
            else
                timer_istart_ms(uart->i.rx_timer, uart->i.rx_interleaved_timeout);
        }
#if (UART_IO_PREBUFFER)
        else
        {
            if (rb_is_full(&uart->i.rx_rb))
                rb_get(&uart->i.rx_rb);
            uart->i.rx_buf[rb_put(&uart->i.rx_rb)] = c;
        }
#endif //UART_IO_PREBUFFER
    }
    else
#endif //UART_IO_MODE_SUPPORT
        ipc_ipost_inline(process_iget_current(), HAL_CMD(HAL_UART, IPC_UART_ISR_RX), port, 0, c);
    return res;
}

static bool lpc_uart_tx_isr(CORE* drv, UART_PORT port, uint8_t* c)
{
    bool res = false;
    UART* uart = drv->uart.uarts[port];
#if (UART_IO_MODE_SUPPORT)
    if (uart->io_mode)
    {
        if (uart->i.tx_io)
        {
            if (uart->i.tx_processed < uart->i.tx_io->data_size)
            {
                *c = ((uint8_t*)io_data(uart->i.tx_io))[uart->i.tx_processed++];
                res = true;
            }
            //no more
            if (uart->i.tx_processed >= uart->i.tx_io->data_size)
            {
                iio_complete(uart->i.tx_process, HAL_IO_CMD(HAL_UART, IPC_WRITE), port, uart->i.tx_io);
                uart->i.tx_io = NULL;
            }
        }
    }
    else
#endif //UART_IO_MODE_SUPPORT
    {
        if (uart->s.tx_chunk_size)
        {
            if (uart->s.tx_chunk_pos < uart->s.tx_chunk_size)
            {
                *c = uart->s.tx_buf[uart->s.tx_chunk_pos++];
                res = true;
            }
            //no more
            if (uart->s.tx_chunk_pos >= uart->s.tx_chunk_size)
            {
                uart->s.tx_chunk_size = 0;
                ipc_ipost_inline(process_iget_current(), HAL_CMD(HAL_UART, IPC_UART_ISR_TX), port, 0, 0);
            }
        }
    }
    return res;
}

void lpc_uart_on_isr(int vector, void* param)
{
    unsigned int lsr;
    int i;
    uint8_t c;
    bool more;
    UART_PORT port = UART_0;
    CORE* drv = (CORE*)param;
    if (vector != __UART_VECTORS[UART_0])
        for (i = 1; i < UARTS_COUNT; ++i)
            if (vector == __UART_VECTORS[i])
            {
                port = i;
                break;
            }
    UART* uart = drv->uart.uarts[port];

    switch (__USART_REGS[port]->IIR & USART0_IIR_INTID_Msk)
    {
    case USART0_IIR_INTID_RLS:
        //decode error, if any
        lsr = __USART_REGS[port]->LSR;
        if (lsr & USART0_LSR_OE_Msk)
            uart->error = ERROR_OVERFLOW;
        if (lsr & USART0_LSR_PE_Msk)
            uart->error = ERROR_INVALID_PARITY;
        if (lsr & USART0_LSR_FE_Msk)
            uart->error = ERROR_INVALID_FRAME;
        if (lsr & USART0_LSR_BI_Msk)
            uart->error = ERROR_COMM_BREAK;
        break;
    case USART0_IIR_INTID_RDA:
        //need more data?
        if (!lpc_uart_rx_isr(drv, port, __USART_REGS[port]->RBR))
            __USART_REGS[port]->IER &= ~USART0_IER_RBRIE_Msk;
        break;
    case USART0_IIR_INTID_THRE:
        //transmit more
        more = true;
        while ((__USART_REGS[port]->LSR & USART0_LSR_THRE_Msk) && (more = lpc_uart_tx_isr(drv, port, &c)))
                __USART_REGS[port]->THR = c;
        //no more? mask interrupt
        if (!more)
            __USART_REGS[port]->IER &= ~USART0_IER_THREIE_Msk;
        break;
    }
}

#ifdef LPC11U6x
void lpc_uart4_on_isr(int vector, void* param)
{
    unsigned int lsr;
    uint8_t c;
    bool more;
    //find port by vector
    UART_PORT port;
    CORE* drv = (CORE*)param;
    if (vector == __UART_VECTORS[1])
        port = UART_1;
    else
        port = UART_3;
    UART* uart = drv->uart.uarts[port];
    for (i = 0; i < 2; ++i)
    {
        port = (vector == __UART_VECTORS[1] ? UART_1 : UART_2) + i * 2;
        if (uart == NULL)
            continue;

        //error condition
        if (__USART1_REGS[port - 1]->STAT & (USART4_STAT_OVERRUNINT | USART4_STAT_FRAMERRINT | USART4_STAT_PARITTERRINT | USART4_STAT_RXNOISEINT | USART4_STAT_DELTARXBRKINT))
        {
            if (__USART1_REGS[port - 1]->STAT & USART4_STAT_OVERRUNINT)
                drv->uart.uarts[port - 1]->error = ERROR_OVERFLOW;
            if (__USART1_REGS[port - 1]->STAT & USART4_STAT_FRAMERRINT)
                drv->uart.uarts[port - 1]->error = ERROR_INVALID_FRAME;
            if (__USART1_REGS[port - 1]->STAT & USART4_STAT_PARITTERRINT)
                drv->uart.uarts[port - 1]->error = ERROR_INVALID_PARITY;
            if (__USART1_REGS[port - 1]->STAT & USART4_STAT_RXNOISEINT)
                drv->uart.uarts[port - 1]->error = ERROR_LINE_NOISE;
            if (__USART1_REGS[port - 1]->STAT & USART4_STAT_DELTARXBRKINT)
                drv->uart.uarts[port - 1]->error = ERROR_COMM_BREAK;
            __USART1_REGS[port - 1]->STAT = USART4_STAT_OVERRUNINT | USART4_STAT_FRAMERRINT | USART4_STAT_PARITTERRINT | USART4_STAT_RXNOISEINT | USART4_STAT_DELTARXBRKINT;
        }
        //ready to rx
        if (__USART1_REGS[port - 1]->STAT & USART4_STAT_RXRDY)
            lpc_uart_rx_isr(drv, port, __USART1_REGS[port - 1]->RXDAT);
        //tx
        if (__USART1_REGS[port - 1]->INTEN & USART4_INTENSET_TXRDYEN)
        {
            more = true;
            while ((__USART1_REGS[port - 1]->STAT & USART4_STAT_TXRDY) &&  (more = lpc_uart_tx_isr(drv, port, &c)))
                __USART1_REGS[port - 1]->TXDAT = c;
            //no more? mask interrupt
            if (!more)
                __USART1_REGS[port - 1]->INTENCLR = USART4_INTENSET_TXRDYEN;
        }
    }
}
#endif

static inline void lpc_uart_set_baudrate(CORE* drv, UART_PORT port, IPC* ipc)
{
    unsigned int clock;
    BAUD baudrate;
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    uart_decode_baudrate(ipc, &baudrate);
#ifdef LPC18xx
    clock = lpc_power_get_clock_inside(POWER_BUS_CLOCK);
#else //LPC11Uxx
    clock = lpc_power_get_core_clock_inside();
#endif //LPC18xx
    unsigned int divider = clock / (((__USART_REGS[port]->OSR >> 4) & 0x7ff) + 1 ) / baudrate.baud;
#ifdef LPC11U6x
    if (port > UART_0)
    {
        __USART1_REGS[port - 1]->CFG = ((baudrate.data_bits - 7) << USART4_CFG_DATALEN_POS) | ((baudrate.stop_bits - 1) << USART4_CFG_STOPLEN_POS);
        switch (baudrate.parity)
        {
        case 'O':
            __USART1_REGS[port - 1]->CFG |= USART4_CFG_PARITYSEL_ODD;
            break;
        case 'E':
            __USART1_REGS[port - 1]->CFG |= USART4_CFG_PARITYSEL_EVEN;
            break;
        }
        __USART1_REGS[port - 1]->BRG = divider & 0xffff;
        __USART1_REGS[port - 1]->CFG |= USART4_CFG_ENABLE
    }
    else
#endif
    {
        __USART_REGS[port]->LCR = ((baudrate.data_bits - 5) << USART0_LCR_WLS_Pos) | ((baudrate.stop_bits - 1) << USART0_LCR_SBS_Pos) | USART0_LCR_DLAB_Msk;
        switch (baudrate.parity)
        {
        case 'O':
            __USART_REGS[port]->LCR |= USART0_LCR_PE_Msk | USART0_LCR_PS_ODD;
            break;
        case 'E':
            __USART_REGS[port]->LCR |= USART0_LCR_PE_Msk | USART0_LCR_PS_EVEN;
            break;
        }
        __USART_REGS[port]->DLM = (divider >> 8) & 0xff;
        __USART_REGS[port]->DLL = divider & 0xff;
        __USART_REGS[port]->LCR &= ~USART0_LCR_DLAB_Msk;
    }
}

static void lpc_uart_destroy(CORE* drv, UART_PORT port)
{
#if (UART_IO_MODE_SUPPORT)
    if (drv->uart.uarts[port]->io_mode)
    {
        timer_destroy(drv->uart.uarts[port]->i.rx_timer);
    }
    else
#endif //UART_IO_MODE_SUPPORT
    {
        stream_close(drv->uart.uarts[port]->s.tx_handle);
        stream_close(drv->uart.uarts[port]->s.rx_handle);
        stream_destroy(drv->uart.uarts[port]->s.tx_stream);
        stream_destroy(drv->uart.uarts[port]->s.rx_stream);
    }
    free(drv->uart.uarts[port]);
    drv->uart.uarts[port] = NULL;
}

static inline bool lpc_uart_open_stream(CORE* drv, UART_PORT port, unsigned int mode)
{
    drv->uart.uarts[port]->s.tx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->s.tx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->s.rx_stream = INVALID_HANDLE;
    drv->uart.uarts[port]->s.rx_handle = INVALID_HANDLE;
    drv->uart.uarts[port]->s.tx_total = 0;
    drv->uart.uarts[port]->s.tx_chunk_pos = drv->uart.uarts[port]->s.tx_chunk_size = 0;
    if (mode & UART_TX_STREAM)
    {
        drv->uart.uarts[port]->s.tx_stream = stream_create(UART_STREAM_SIZE);
        drv->uart.uarts[port]->s.tx_handle = stream_open(drv->uart.uarts[port]->s.tx_stream);
        stream_listen(drv->uart.uarts[port]->s.tx_stream, port, HAL_UART);
    }
    if (mode & UART_RX_STREAM)
    {
        drv->uart.uarts[port]->s.rx_stream = stream_create(UART_STREAM_SIZE);
        drv->uart.uarts[port]->s.rx_handle = stream_open(drv->uart.uarts[port]->s.rx_stream);
        drv->uart.uarts[port]->s.rx_free = stream_get_free(drv->uart.uarts[port]->s.rx_stream);
    }
    if (((mode & UART_TX_STREAM) && (drv->uart.uarts[port]->s.tx_handle == INVALID_HANDLE)) ||
        ((mode & UART_RX_STREAM) && (drv->uart.uarts[port]->s.rx_handle == INVALID_HANDLE)))
        return false;
    if (mode & UART_RX_STREAM)
        __USART_REGS[port]->IER |= USART0_IER_RBRIE_Msk;
    return true;
}

#if (UART_IO_MODE_SUPPORT)
static inline bool lpc_uart_open_io(CORE* drv, UART_PORT port)
{
    UART_IO* i = &(drv->uart.uarts[port]->i);
    i->tx_io = i->rx_io = NULL;
    i->rx_timer = timer_create(port, HAL_UART);
    i->rx_char_timeout = UART_CHAR_TIMEOUT_MS;
    i->rx_interleaved_timeout = UART_INTERLEAVED_TIMEOUT_MS;
#if (UART_IO_PREBUFFER)
    rb_init(&i->rx_rb, UART_BUF_SIZE);
#endif //UART_IO_PREBUFFER
    return i->rx_timer != INVALID_HANDLE;
}
#endif //UART_IO_MODE_SUPPORT

static inline void lpc_uart_open(CORE* drv, UART_PORT port, unsigned int mode)
{
    bool ok;
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
    drv->uart.uarts[port]->io_mode = ((mode & UART_MODE) == UART_MODE_IO);

    if (drv->uart.uarts[port]->io_mode)
    {
#if (UART_IO_MODE_SUPPORT)
        ok = lpc_uart_open_io(drv, port);
#else
        ok = false;
#endif //UART_IO_MODE_SUPPORT
    }
    else
        ok = lpc_uart_open_stream(drv, port, mode);
    if (!ok)
    {
        lpc_uart_destroy(drv, port);
        return;
    }

    //power up
#ifdef LPC11Uxx
    LPC_SYSCON->SYSAHBCLKCTRL |= 1 << __UART_POWER_PINS[port];
#else //LPC18xx
    //map PLL1 output to UART clock
    ((uint32_t*)LPC_CGU_UART0_CLOCK_BASE)[port] = CGU_BASE_UART0_CLK_PD_Msk;
    ((uint32_t*)LPC_CGU_UART0_CLOCK_BASE)[port] |= CGU_CLK_PLL1;
    ((uint32_t*)LPC_CGU_UART0_CLOCK_BASE)[port] &= ~CGU_BASE_UART0_CLK_PD_Msk;
#endif //LPC11Uxx
    //remove reset state. Only for LPC11U6x
#ifdef LPC11U6x
    if (port > UART_0)
    {
        LPC_SYSCON->PRESETCTRL |= 1 << __UART_RESET_PINS[port - 1];
        if (mode & UART_RX_STREAM)
            __USART1_REGS[port - 1]->INTENCSET = USART4_INTENSET_RXRDYEN | USART4_INTENSET_OVERRUNEN | USART4_INTENSET_FRAMERREN
                                          | USART4_INTENSET_PARITTERREN | USART4_INTENSET_RXNOISEEN | USART4_INTENSET_DELTARXBRKEN;
    }
    else
#endif
    {
         //enable FIFO
        __USART_REGS[port]->FCR = USART0_FCR_FIFOEN_Msk | USART0_FCR_TXFIFORES_Msk | USART0_FCR_RXFIFORES_Msk;
    }

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
    NVIC_EnableIRQ(__UART_VECTORS[port]);
    NVIC_SetPriority(__UART_VECTORS[port], 2);
}

static void lpc_uart_flush(CORE* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
#ifdef LPC11U6x
    if (port > UART_0)
    {
        __USART1_REGS[port - 1]->INTENCLR = USART4_INTENSET_TXRDYEN;
    }
    else
#endif
    {
        __USART_REGS[port]->IER &= ~USART0_IER_THREIE_Msk;
        __USART_REGS[port]->FCR = USART0_FCR_TXFIFORES_Msk | USART0_FCR_RXFIFORES_Msk;
    }
#if (UART_IO_MODE_SUPPORT)
    if (drv->uart.uarts[port]->io_mode)
    {
        IO* rx_io;
        IO* tx_io;
        __disable_irq();
        rx_io = drv->uart.uarts[port]->i.rx_io;
        drv->uart.uarts[port]->i.rx_io = NULL;
        tx_io = drv->uart.uarts[port]->i.tx_io;
        drv->uart.uarts[port]->i.tx_io = NULL;
#if (UART_IO_PREBUFFER)
        rb_clear(&drv->uart.uarts[port]->i.rx_rb);
#endif //UART_IO_PREBUFFER
        __enable_irq();
        if (rx_io)
            io_complete_ex(drv->uart.uarts[port]->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, rx_io, ERROR_IO_CANCELLED);
        if (tx_io)
            io_complete_ex(drv->uart.uarts[port]->i.tx_process, HAL_IO_CMD(HAL_UART, IPC_WRITE), port, tx_io, ERROR_IO_CANCELLED);
        timer_stop(drv->uart.uarts[port]->i.rx_timer, port, HAL_UART);
        __USART_REGS[port]->IER &= ~USART0_IER_RBRIE_Msk;
    }
    else
#endif //UART_IO_MODE_SUPPORT
    {
        if (drv->uart.uarts[port]->s.tx_stream != INVALID_HANDLE)
        {
            stream_flush(drv->uart.uarts[port]->s.tx_stream);
            stream_listen(drv->uart.uarts[port]->s.tx_stream, port, HAL_UART);
            drv->uart.uarts[port]->s.tx_total = 0;
            __disable_irq();
            drv->uart.uarts[port]->s.tx_chunk_pos = drv->uart.uarts[port]->s.tx_chunk_size = 0;
            __enable_irq();
        }
        if (drv->uart.uarts[port]->s.rx_stream != INVALID_HANDLE)
            stream_flush(drv->uart.uarts[port]->s.rx_stream);
    }
    drv->uart.uarts[port]->error = ERROR_OK;
}

static inline void lpc_uart_close(CORE* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
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
         __USART_REGS[port]->FCR &= ~USART0_FCR_FIFOEN_Msk;
    }
#ifdef LPC11Uxx
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << __UART_POWER_PINS[port]);
#else
    ((uint32_t*)LPC_CGU_UART0_CLOCK_BASE)[port] = CGU_BASE_UART0_CLK_PD_Msk;
#endif //LPC11Uxx

    lpc_uart_flush(drv, port);
    lpc_uart_destroy(drv, port);
}

static inline HANDLE lpc_uart_get_tx_stream(CORE* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return INVALID_HANDLE;
    }
#if (UART_IO_MODE_SUPPORT)
    if (drv->uart.uarts[port]->io_mode)
    {
        error(ERROR_INVALID_MODE);
        return INVALID_HANDLE;
    }
#endif //UART_IO_MODE_SUPPORT
    return drv->uart.uarts[port]->s.tx_stream;
}

static inline HANDLE lpc_uart_get_rx_stream(CORE* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return INVALID_HANDLE;
    }
#if (UART_IO_MODE_SUPPORT)
    if (drv->uart.uarts[port]->io_mode)
    {
        error(ERROR_INVALID_MODE);
        return INVALID_HANDLE;
    }
#endif //UART_IO_MODE_SUPPORT
    return drv->uart.uarts[port]->s.rx_stream;
}

static inline uint16_t lpc_uart_get_last_error(CORE* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return ERROR_OK;
    }
    return drv->uart.uarts[port]->error;
}

static inline void lpc_uart_clear_error(CORE* drv, UART_PORT port)
{
    if (drv->uart.uarts[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    drv->uart.uarts[port]->error = ERROR_OK;
}

void uart_write_kernel(const char *const buf, unsigned int size, void* param);

static inline void lpc_uart_write(CORE* drv, UART_PORT port, unsigned int total)
{
    if (total)
        drv->uart.uarts[port]->s.tx_total = total;
    if (drv->uart.uarts[port]->s.tx_total == 0)
        drv->uart.uarts[port]->s.tx_total = stream_get_size(drv->uart.uarts[port]->s.tx_stream);
    if (drv->uart.uarts[port]->s.tx_total)
    {
        unsigned int to_read = drv->uart.uarts[port]->s.tx_total;
        if (drv->uart.uarts[port]->s.tx_total > UART_BUF_SIZE)
            to_read = UART_BUF_SIZE;
        if (stream_read(drv->uart.uarts[port]->s.tx_handle, drv->uart.uarts[port]->s.tx_buf, to_read))
        {
            drv->uart.uarts[port]->s.tx_chunk_pos = 1;
            drv->uart.uarts[port]->s.tx_total -= to_read;
            //prepare
#ifdef LPC11U6x
            if (port > UART_0)
                __USART1_REGS[port - 1]->TXDAT = drv->uart.uarts[port]->s.tx_buf[0];
            else
#endif
                __USART_REGS[port]->THR = drv->uart.uarts[port]->s.tx_buf[0];
            //this will enable isr processing, if printd/printk was called during setup
            drv->uart.uarts[port]->s.tx_chunk_size = to_read;
            //start
#ifdef LPC11U6x
            if (port > UART_0)
                __USART1_REGS[port - 1]->INTENSET = USART4_INTENSET_TXRDYEN;
            else
#endif
                __USART_REGS[port]->IER |= USART0_IER_THREIE_Msk;
        }
    }
    else
        stream_listen(drv->uart.uarts[port]->s.tx_stream, port, HAL_UART);

}

static inline void lpc_uart_read(CORE* drv, UART_PORT port, char c)
{
    //caching calls to svc
    if (drv->uart.uarts[port]->s.rx_free == 0)
        (drv->uart.uarts[port]->s.rx_free = stream_get_free(drv->uart.uarts[port]->s.rx_stream));
    //if stream is full, char will be discarded
    if (drv->uart.uarts[port]->s.rx_free)
    {
        stream_write(drv->uart.uarts[port]->s.rx_handle, &c, 1);
        drv->uart.uarts[port]->s.rx_free--;
    }
}

#if (UART_IO_MODE_SUPPORT)
static inline void lpc_uart_io_read(CORE* drv, UART_PORT port, IPC* ipc)
{
    UART* uart = drv->uart.uarts[port];
    IO* io;
    if (uart == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (!uart->io_mode)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if (uart->i.rx_io)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    io = (IO*)ipc->param2;
    uart->i.rx_process = ipc->process;
    uart->i.rx_max = ipc->param3;
    io->data_size = 0;
#if (UART_IO_PREBUFFER)
    __disable_irq();
    while (!rb_is_empty(&uart->i.rx_rb) && (io->data_size < uart->i.rx_max))
        ((uint8_t*)io_data(io))[io->data_size++] = uart->i.rx_buf[rb_get(&uart->i.rx_rb)];
    __enable_irq();
#endif //UART_IO_PREBUFFER
    uart->i.rx_io = io;
    __USART_REGS[port]->IER |= USART0_IER_RBRIE_Msk;
#if (UART_IO_PREBUFFER)
    timer_start_ms(uart->i.rx_timer, io->data_size ? uart->i.rx_interleaved_timeout : uart->i.rx_char_timeout);
#else
    timer_start_ms(uart->i.rx_timer, uart->i.rx_char_timeout);
#endif //UART_IO_PREBUFFER
    error(ERROR_SYNC);
}

static inline void lpc_uart_io_write(CORE* drv, UART_PORT port, IPC* ipc)
{
    UART* uart = drv->uart.uarts[port];
    IO* io;
    if (uart == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (!uart->io_mode)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if (uart->i.tx_io)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    io = (IO*)ipc->param2;
    uart->i.tx_process = ipc->process;
    uart->i.tx_processed = 1;
    //prepare
#ifdef LPC11U6x
    if (port > UART_0)
        __USART1_REGS[port - 1]->TXDAT = ((uint8_t*)io_data(io))[0];
    else
#endif
        __USART_REGS[port]->THR = ((uint8_t*)io_data(io))[0];
    //this will enable isr processing, if printd/printk was called during setup
    uart->i.tx_io = io;
    //start
#ifdef LPC11U6x
    if (port > UART_0)
        __USART1_REGS[port - 1]->INTENSET = USART4_INTENSET_TXRDYEN;
    else
#endif
        __USART_REGS[port]->IER |= USART0_IER_THREIE_Msk;
    error(ERROR_SYNC);
}

static inline void lpc_uart_io_read_timeout(CORE* drv, UART_PORT port)
{
    IO* io = NULL;
    UART* uart = drv->uart.uarts[port];
    timer_stop(uart->i.rx_timer, port, HAL_UART);
    __disable_irq();
    if (uart->i.rx_io)
    {
        io = uart->i.rx_io;
        uart->i.rx_io = NULL;
    }
    __enable_irq();
    if (io)
    {
        if (io->data_size)
            io_complete(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, io);
        else
            //no data? timeout
            io_complete_ex(uart->i.rx_process, HAL_IO_CMD(HAL_UART, IPC_READ), port, io, ERROR_TIMEOUT);
    }
}
#endif //UART_IO_MODE_SUPPORT

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
            while ((__USART1_REGS[port - 1]->STAT & USART4_STAT_TXRDY) == 0) {}
            __USART1_REGS[port - 1]->TXDAT = buf[i];
        }
        else
#endif
        {
            while ((__USART_REGS[port]->LSR & USART0_LSR_THRE_Msk) == 0) {}
                __USART_REGS[port]->THR = buf[i];
        }
    }
    NVIC_EnableIRQ(__UART_VECTORS[port]);
}

static inline void lpc_uart_setup_printk(CORE* drv, UART_PORT port)
{
    //setup kernel printk dbg
    setup_dbg(uart_write_kernel, (void*)port);
}

void lpc_uart_init(CORE* drv)
{
    int i;
#if defined(LPC11U6x)
    LPC_SYSCON->USART0CLKDIV = 1;
    LPC_SYSCON->FRGCLKDIV = 1;
    drv->uart.uart13 = drv->uart.uart24 = 0;
#elif defined(LPC11Uxx)
    LPC_SYSCON->UARTCLKDIV = 1;
#endif
    for (i = 0; i < UARTS_COUNT; ++i)
        drv->uart.uarts[i] = NULL;
}

void lpc_uart_request(CORE* drv, IPC* ipc)
{
    UART_PORT port = ipc->param1;
    if (port >= UARTS_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_uart_open(drv, port, ipc->param2);
        break;
    case IPC_CLOSE:
        lpc_uart_close(drv, port);
        break;
    case IPC_UART_SET_BAUDRATE:
        lpc_uart_set_baudrate(drv, port, ipc);
        break;
    case IPC_FLUSH:
        lpc_uart_flush(drv, port);
        break;
    case IPC_GET_TX_STREAM:
        ipc->param2 = lpc_uart_get_tx_stream(drv, port);
        break;
    case IPC_GET_RX_STREAM:
        ipc->param2 = lpc_uart_get_rx_stream(drv, port);
        break;
    case IPC_UART_GET_LAST_ERROR:
        ipc->param2 = lpc_uart_get_last_error(drv, port);
        break;
    case IPC_UART_CLEAR_ERROR:
        lpc_uart_clear_error(drv, port);
        break;
    case IPC_UART_SETUP_PRINTK:
        lpc_uart_setup_printk(drv, port);
        break;
    case IPC_STREAM_WRITE:
    case IPC_UART_ISR_TX:
        lpc_uart_write(drv, port, ipc->param3);
        //message from kernel (or ISR), no response
        break;
    case IPC_UART_ISR_RX:
        lpc_uart_read(drv, port, ipc->param3);
        //message from ISR, no response
        break;
#if (UART_IO_MODE_SUPPORT)
    case IPC_READ:
        lpc_uart_io_read(drv, port, ipc);
        break;
    case IPC_WRITE:
        lpc_uart_io_write(drv, port, ipc);
        break;
    case IPC_TIMEOUT:
        lpc_uart_io_read_timeout(drv, port);
        break;
#endif //UART_IO_MODE_SUPPORT
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
