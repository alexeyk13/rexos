/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_uart.h"
#include "nrf_power.h"
#include "../../userspace/sys.h"
#include "../kerror.h"
#include "../kstdlib.h"
#include "../ksystime.h"
#include "../kstream.h"
#include "../kirq.h"
#include "../kexo.h"
#include <string.h>
#include "nrf_exo_private.h"
#include "sys_config.h"

typedef NRF_UART_Type*      NRF_UART_Type_P;

#if defined(NRF51)
static const unsigned int UART_VECTORS[UARTS_COUNT] = {UART0_IRQn};
static const NRF_UART_Type_P UART_REGS[UARTS_COUNT] = {NRF_UART0};
#endif // NRF51


void nrf_uart_on_isr(int vector, void* param)
{

}

void uart_write_kernel(const char *const buf, unsigned int size, void* param)
{
    NVIC_DisableIRQ(UART_VECTORS[(UART_PORT)param]);
    int i;
    // disable uart irq
    UART_REGS[(UART_PORT)param]->INTENCLR |= UART_INTENCLR_TXDRDY_Msk;
    for(i = 0; i < size; ++i)
    {
        // send new data
        UART_REGS[(UART_PORT)param]->TXD = buf[i];
        // wait previous TX complete
        while(UART_REGS[(UART_PORT)param]->EVENTS_TXDRDY != 1);
        // clear flag
        UART_REGS[(UART_PORT)param]->EVENTS_TXDRDY = 0;
    }
    // enable uart irq
    UART_REGS[(UART_PORT)param]->INTENSET |= UART_INTENSET_TXDRDY_Msk;
    //transmitter will be disabled in next IRQ TC
    NVIC_EnableIRQ(UART_VECTORS[(UART_PORT)param]);
}

static inline void nrf_uart_set_baudrate(EXO* exo, UART_PORT port, IPC* ipc)
{
    BAUD baudrate;
    unsigned int clock, stop;

    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    uart_decode_baudrate(ipc, &baudrate);

    switch(baudrate.baud)
    {
        case 115200:
            UART_REGS[port]->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud115200 << UART_BAUDRATE_BAUDRATE_Pos);
            break;
        default:
            UART_REGS[port]->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud9600 << UART_BAUDRATE_BAUDRATE_Pos);
            break;
    }

    if('N' != baudrate.parity)
        UART_REGS[port]->CONFIG |= UART_CONFIG_PARITY_Msk;
}

static inline void nrf_uart_setup_printk(EXO* exo, UART_PORT port)
{
    //setup kernel printk dbg
    kernel_setup_dbg(uart_write_kernel, (void*)port);
}

static inline void nrf_uart_open(EXO* exo, UART_PORT port, unsigned int mode)
{
//    bool ok;
    if (exo->uart.uarts[port] != NULL)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    exo->uart.uarts[port] = kmalloc(sizeof(UART));
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_OUT_OF_MEMORY);
        return;
    }

    exo->uart.uarts[port]->error = ERROR_OK;
    exo->uart.uarts[port]->io_mode = ((mode & UART_MODE) == UART_MODE_IO);

    // TODO: IO mode later
//    if (exo->uart.uarts[port]->io_mode)
//    {
//#if (UART_IO_MODE_SUPPORT)
//        ok = lpc_uart_open_io(exo, port);
//#else
//        ok = false;
//#endif //UART_IO_MODE_SUPPORT
//    }
//    else
//        ok = lpc_uart_open_stream(exo, port, mode);
//    if (!ok)
//    {
//        lpc_uart_destroy(exo, port);
//        return;
//    }

    //power up

    // set TX_PIN
    UART_REGS[port]->PSELTXD = P9; // TODO: temporary UART pin define
//    NRF_UART0->PSELRXD = UART_RX_PIN;

    UART_REGS[port]->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    UART_REGS[port]->EVENTS_RXDRDY = 0;

    // Enable UART RX interrupt only
    //NRF_UART0->INTENSET = (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);

    // Start reception and transmission
    UART_REGS[port]->TASKS_STARTTX = 1;
}

static inline void nrf_uart_close(EXO* exo, UART_PORT port)
{
    if (exo->uart.uarts[port] == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    //disable interrupts
    NVIC_DisableIRQ(UART_VECTORS[port]);
    kirq_unregister(KERNEL_HANDLE, UART_VECTORS[port]);

    // uart destroy
    kfree(exo->uart.uarts[port]);
    exo->uart.uarts[port] = NULL;
}

void nrf_uart_request(EXO* exo, IPC* ipc)
{
    UART_PORT port = (UART_PORT)ipc->param1;
    if (port >= UARTS_COUNT)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        nrf_uart_open(exo, port, ipc->param2);
        break;
    case IPC_CLOSE:
        break;
    case IPC_UART_SET_BAUDRATE:
        nrf_uart_set_baudrate(exo, port, ipc);
        break;
    case IPC_FLUSH:
        break;
    case IPC_GET_TX_STREAM:
        break;
    case IPC_GET_RX_STREAM:
        break;
    case IPC_UART_GET_LAST_ERROR:
        break;
    case IPC_UART_CLEAR_ERROR:
        break;
    case IPC_UART_SETUP_PRINTK:
        nrf_uart_setup_printk(exo, port);
        break;
    case IPC_STREAM_WRITE:
        break;
#if (UART_IO_MODE_SUPPORT)
    case IPC_READ:
        break;
    case IPC_WRITE:
        break;
    case IPC_TIMEOUT:
        break;
    case IPC_UART_SET_COMM_TIMEOUTS:
        break;
#endif //UART_IO_MODE_SUPPORT
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void nrf_uart_init(EXO* exo)
{
    int i;
    for (i = 0; i < UARTS_COUNT; ++i)
        exo->uart.uarts[i] = NULL;
}
