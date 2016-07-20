/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_i2c.h"
#include "stm32_core_private.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/irq.h"
#include "../../userspace/stm32/stm32_driver.h"

#define I2C_CR2_NBYTES_Pos                              16
#define I2C_CR2_NBYTES_Msk                              (0xff << 16)


typedef I2C_TypeDef* I2C_TypeDef_P;
#if (I2C_COUNT > 1)
static const I2C_TypeDef_P __I2C_REGS[] =               {I2C1, I2C2};
static const uint8_t __I2C_VECTORS[] =                  {23, 24};
static const uint8_t __I2C_POWER_PINS[] =               {21, 22};
#else
static const I2C_TypeDef_P __I2C_REGS[] =               {I2C1};
static const uint8_t __I2C_VECTORS[] =                  {23};
static const uint8_t __I2C_POWER_PINS[] =               {21};
#endif

//values from datasheet
#define I2C_TIMING_NORMAL_SPEED                         ((1 << 28) | (0x13 << 0) | (0xf << 8) | (0x2 << 16) | (0x4 << 20))
#define I2C_TIMING_FAST_SPEED                           ((0 << 28) | (0x9 << 0) | (0x3 << 8) | (0x1 << 16) | (0x3 << 20))
#define I2C_TIMING_FAST_PLUS_SPEED                      ((0 << 28) | (0x6 << 0) | (0x3 << 8) | (0x0 << 16) | (0x1 << 20))

void stm32_i2c_init(CORE* core)
{
    int i;
    for (i = 0; i < I2C_COUNT; ++i)
        core->i2c.i2cs[i] = NULL;
}

static void stm32_i2c_tx_byte(I2C_PORT port)
{
    __I2C_REGS[port]->CR2 = (__I2C_REGS[port]->CR2 & ~(I2C_CR2_NBYTES_Msk | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND | I2C_CR2_RELOAD)) |
                            (1 << I2C_CR2_NBYTES_Pos);
}

static void stm32_i2c_tx_byte_more(I2C_PORT port)
{
    __I2C_REGS[port]->CR2 = (__I2C_REGS[port]->CR2 & ~(I2C_CR2_NBYTES_Msk | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND)) |
                            (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_RELOAD;
}

static void stm32_i2c_rx_byte(I2C_PORT port)
{
    __I2C_REGS[port]->CR2 = (__I2C_REGS[port]->CR2 & ~(I2C_CR2_NBYTES_Msk | I2C_CR2_AUTOEND)) |
                            (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN | I2C_CR2_RELOAD;
}

static void stm32_i2c_tx_data(I2C* i2c, I2C_PORT port)
{
    __I2C_REGS[port]->CR2 = (__I2C_REGS[port]->CR2 & ~(I2C_CR2_NBYTES_Msk | I2C_CR2_RELOAD | I2C_CR2_RD_WRN)) |
                            (((i2c->io->data_size) & 0xff) << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND;
}

static void stm32_i2c_rx_data(I2C* i2c, I2C_PORT port)
{
    __I2C_REGS[port]->CR2 = (__I2C_REGS[port]->CR2 & ~(I2C_CR2_NBYTES_Msk | I2C_CR2_RELOAD)) |
                            (((i2c->size) & 0xff) << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
}

static void stm32_i2c_on_error_isr(I2C* i2c, I2C_PORT port, int error)
{
    //soft reset
    __I2C_REGS[port]->CR1 &= ~I2C_CR1_PE;
    __NOP();
    __NOP();
    __NOP();
    __I2C_REGS[port]->CR1 |= I2C_CR1_PE;
    __I2C_REGS[port]->ICR = I2C_ICR_NACKCF;
    iio_complete_ex(i2c->process, HAL_IO_CMD(HAL_I2C, (i2c->io_mode == I2C_IO_MODE_TX) ? IPC_WRITE : IPC_READ), port, i2c->io, error);
    i2c->io_mode = I2C_IO_MODE_IDLE;
}

static inline void stm32_i2c_on_rx_isr(I2C* i2c, I2C_PORT port)
{
    unsigned int sr = __I2C_REGS[port]->ISR;
    switch (i2c->state)
    {
    //transmitting address, Rs follow
    case I2C_STATE_ADDR:
        if (sr & I2C_ISR_TXIS)
            __I2C_REGS[port]->TXDR = i2c->stack->addr;
        else if (sr & I2C_ISR_TC)
        {
            if (i2c->stack->flags & I2C_FLAG_LEN)
            {
                i2c->state = I2C_STATE_LEN;
                stm32_i2c_rx_byte(port);
            }
            else
            {
                i2c->state = I2C_STATE_DATA;
                stm32_i2c_rx_data(i2c, port);
            }
            //set START
            __I2C_REGS[port]->CR2 |= I2C_CR2_START;
        }
        else
            stm32_i2c_on_error_isr(i2c, port, ERROR_NAK);
        break;
    //received len
    case I2C_STATE_LEN:
        if (sr & I2C_ISR_RXNE)
        {
            i2c->size = __I2C_REGS[port]->RXDR & 0xff;
            i2c->state = I2C_STATE_DATA;
            stm32_i2c_rx_data(i2c, port);
        }
        else
            stm32_i2c_on_error_isr(i2c, port, ERROR_NAK);
        break;
    case I2C_STATE_DATA:
        if ((sr & I2C_ISR_RXNE))
        {
            ((uint8_t*)io_data(i2c->io))[i2c->io->data_size++] = __I2C_REGS[port]->RXDR & 0xff;
            if (i2c->io->data_size >= i2c->size)
            {
                iio_complete(i2c->process, HAL_IO_CMD(HAL_I2C, IPC_READ), port, i2c->io);
                i2c->io_mode = I2C_IO_MODE_IDLE;
            }
        }
        else
            stm32_i2c_on_error_isr(i2c, port, ERROR_NAK);
        break;
    }
}

static inline void stm32_i2c_on_tx_isr(I2C* i2c, I2C_PORT port)
{
    unsigned int sr = __I2C_REGS[port]->ISR;
    switch (i2c->state)
    {
    //transmitting address, Rs follow
    case I2C_STATE_ADDR:
        if (sr & I2C_ISR_TXIS)
            __I2C_REGS[port]->TXDR = i2c->stack->addr;
        else if (sr & I2C_ISR_TCR)
        {
            if (i2c->stack->flags & I2C_FLAG_LEN)
            {
                i2c->state = I2C_STATE_LEN;
                stm32_i2c_tx_byte_more(port);
            }
            else
            {
                i2c->state = I2C_STATE_DATA;
                stm32_i2c_tx_data(i2c, port);
            }
        }
        else
            stm32_i2c_on_error_isr(i2c, port, ERROR_NAK);
        break;
    //received len
    case I2C_STATE_LEN:
        if (sr & I2C_ISR_TXIS)
            __I2C_REGS[port]->TXDR = i2c->io->data_size;
        else if (sr & I2C_ISR_TCR)
        {
            i2c->state = I2C_STATE_DATA;
            stm32_i2c_tx_data(i2c, port);
        }
        else
            stm32_i2c_on_error_isr(i2c, port, ERROR_NAK);
        break;
    case I2C_STATE_DATA:
        if (sr & I2C_ISR_TXIS)
        {
            __I2C_REGS[port]->TXDR = ((uint8_t*)io_data(i2c->io))[i2c->size++];
            if (i2c->size >= i2c->io->data_size)
            {
                iio_complete(i2c->process, HAL_IO_CMD(HAL_I2C, IPC_WRITE), port, i2c->io);
                i2c->io_mode = I2C_IO_MODE_IDLE;
            }
        }
        else
        {
            stm32_i2c_on_error_isr(i2c, port, ERROR_NAK);
        }
        break;
    }
}

void stm32_i2c_on_isr(int vector, void* param)
{
    I2C_PORT port;
    I2C* i2c;
    CORE* core = param;
    port = I2C_1;
#if (I2C_COUNT > 1)
    if (vector != __I2C_VECTORS[0])
        port = I2C_2;
#endif
    i2c = core->i2c.i2cs[port];

    if (__I2C_REGS[port]->ISR & (I2C_ISR_ARLO | I2C_ISR_BERR))
    {
        stm32_i2c_on_error_isr(i2c, port, ERROR_HARDWARE);
        return;
    }


    switch (i2c->io_mode)
    {
    case I2C_IO_MODE_TX:
        stm32_i2c_on_tx_isr(i2c, port);
        break;
    case I2C_IO_MODE_RX:
        stm32_i2c_on_rx_isr(i2c, port);
        break;
    default:
        stm32_i2c_on_error_isr(i2c, port, ERROR_INVALID_STATE);
    }
}

void stm32_i2c_open(CORE* core, I2C_PORT port, unsigned int mode, unsigned int speed)
{
    I2C* i2c = core->i2c.i2cs[port];
    if (i2c)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    i2c = malloc(sizeof(I2C));
    core->i2c.i2cs[port] = i2c;
    if (i2c == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
    i2c->io = NULL;
    i2c->io_mode = I2C_IO_MODE_IDLE;

    //enable clock
    RCC->APB1ENR |= (1 << __I2C_POWER_PINS[port]);

    if (speed <= I2C_NORMAL_CLOCK)
        __I2C_REGS[port]->TIMINGR = I2C_TIMING_NORMAL_SPEED;
    else if (speed <= I2C_FAST_CLOCK)
        __I2C_REGS[port]->TIMINGR = I2C_TIMING_FAST_SPEED;
    else
        __I2C_REGS[port]->TIMINGR = I2C_TIMING_FAST_PLUS_SPEED;

    __I2C_REGS[port]->CR1 |= I2C_CR1_PE | I2C_CR1_ERRIE | I2C_CR1_TCIE | I2C_CR1_NACKIE |  I2C_CR1_RXIE | I2C_CR1_TXIE;

    //enable interrupt
    irq_register(__I2C_VECTORS[port], stm32_i2c_on_isr, (void*)core);
    NVIC_EnableIRQ(__I2C_VECTORS[port]);
    NVIC_SetPriority(__I2C_VECTORS[port], 13);
}

void stm32_i2c_close(CORE* core, I2C_PORT port)
{
    I2C* i2c = core->i2c.i2cs[port];
    if (i2c == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //disable interrupt
    NVIC_DisableIRQ(__I2C_VECTORS[port]);
    irq_unregister(__I2C_VECTORS[port]);

    __I2C_REGS[port]->CR1 &= ~I2C_CR1_PE;
    RCC->APB1ENR &= ~(1 << __I2C_POWER_PINS[port]);

    free(i2c);
    core->i2c.i2cs[port] = NULL;
}

static void stm32_i2c_io(CORE* core, IPC* ipc, bool read)
{
    I2C_PORT port = (I2C_PORT)ipc->param1;
    I2C* i2c = core->i2c.i2cs[port];
    if (i2c == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (i2c->io_mode != I2C_IO_MODE_IDLE)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    i2c->process = ipc->process;
    i2c->io = (IO*)ipc->param2;
    i2c->stack = io_stack(i2c->io);

    if (read)
    {
        i2c->io->data_size = 0;
        i2c->size = ipc->param3;
        i2c->io_mode = I2C_IO_MODE_RX;
    }
    else
    {
        i2c->size = 0;
        i2c->io_mode = I2C_IO_MODE_TX;
    }

    while (__I2C_REGS[port]->ISR & I2C_ISR_BUSY) {}
    __I2C_REGS[port]->ICR = I2C_ICR_STOPCF;
    __I2C_REGS[port]->CR2 = (i2c->stack->sla << 1);
    if (i2c->stack->flags & I2C_FLAG_ADDR)
    {
        i2c->state = I2C_STATE_ADDR;
        read ? stm32_i2c_tx_byte(port) : stm32_i2c_tx_byte_more(port);
    }
    else if (i2c->stack->flags & I2C_FLAG_LEN)
    {
        i2c->state = I2C_STATE_LEN;
        read ? stm32_i2c_rx_byte(port) : stm32_i2c_tx_byte_more(port);
    }
    else
    {
        i2c->state = I2C_STATE_DATA;
        read ? stm32_i2c_rx_data(i2c, port) : stm32_i2c_tx_data(i2c, port);
    }

    //set START
    __I2C_REGS[port]->CR2 |= I2C_CR2_START;
    //all rest in isr
    error(ERROR_SYNC);
}

void stm32_i2c_request(CORE* core, IPC* ipc)
{
    I2C_PORT port = (I2C_PORT)ipc->param1;
    if (port >= I2C_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_i2c_open(core, port, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        stm32_i2c_close(core, port);
        break;
    case IPC_WRITE:
        stm32_i2c_io(core, ipc, false);
        break;
    case IPC_READ:
        stm32_i2c_io(core, ipc, true);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
