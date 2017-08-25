/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_i2c.h"
#include "stm32_exo_private.h"
#include "../kstdlib.h"
#include "../kirq.h"
#include "../kheap.h"
#include "../karray.h"
#include "../../userspace/stm32/stm32_driver.h"

#include "stm32_pin.h"
#include "../../userspace/gpio.h"
#include <string.h>
#include <stdbool.h>

//#define STM32F1 // TODO: remove!!

typedef I2C_TypeDef* I2C_TypeDef_P;
#ifdef STM32F1
#if (I2C_COUNT > 1)
static const I2C_TypeDef_P __I2C_REGS[] =                             {I2C1, I2C2};
static const uint8_t __I2C_VECTORS[] =                                {31, 33};
static const uint8_t __I2C_ERROR_VECTORS[] =                          {32, 34};
static const uint8_t __I2C_POWER_PINS[] =                             {21, 22};
#else
static const I2C_TypeDef_P __I2C_REGS[] =                             {I2C1};
static const uint8_t __I2C_VECTORS[] =                                {31};
static const uint8_t __I2C_ERROR_VECTORS[] =                          {32};
static const uint8_t __I2C_POWER_PINS[] =                             {21};
#endif //I2C_COUNT

#define I2C_SR_ERROR_MASK (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | I2C_SR1_OVR |   \
                           I2C_SR1_PECERR | I2C_SR1_TIMEOUT | I2C_SR1_SMBALERT)

#else
#define I2C_CR2_NBYTES_Pos                                              16
#define I2C_CR2_NBYTES_Msk                                              (0xff << 16)

#if (I2C_COUNT > 1)
static const I2C_TypeDef_P __I2C_REGS[] =                               {I2C1, I2C2};
static const uint8_t __I2C_VECTORS[] =                                  {23, 24};
static const uint8_t __I2C_POWER_PINS[] =                               {21, 22};
#else
static const I2C_TypeDef_P __I2C_REGS[] =                               {I2C1};
static const uint8_t __I2C_VECTORS[] =                                  {23};
static const uint8_t __I2C_POWER_PINS[] =                               {21};
#endif

//values from datasheet
#define I2C_TIMING_NORMAL_SPEED                                         ((1 << 28) | (0x13 << 0) | (0xf << 8) | (0x2 << 16) | (0x4 << 20))
#define I2C_TIMING_FAST_SPEED                                           ((0 << 28) | (0x9 << 0) | (0x3 << 8) | (0x1 << 16) | (0x3 << 20))
#define I2C_TIMING_FAST_PLUS_SPEED                                      ((0 << 28) | (0x6 << 0) | (0x3 << 8) | (0x0 << 16) | (0x1 << 20))
#endif //STM32F1

//---works with slave registers
static inline void regs_init(I2C* i2c)
{
    karray_create(&i2c->regs, sizeof(REG), 1);
}

static inline void regs_free(I2C* i2c)
{
    int i;
    REG* reg;
    if (i2c->regs == NULL)
        return;
    for (i = 0; i < karray_size(i2c->regs); i++)
    {
        reg = (REG*)karray_at(i2c->regs, i);
        kfree(reg->data);
    }
    karray_destroy(&i2c->regs);
}

static inline void regs_delete(I2C* i2c, uint8_t addr)
{
    int i;
    REG* reg;
    for (i = 0; i < karray_size(i2c->regs); i++)
    {
        reg = (REG*)karray_at(i2c->regs, i);
        if (reg == NULL)
            return;
        if (reg->addr == addr)
        {
            kfree(reg->data);
            karray_remove(&i2c->regs, i);
            return;
        }
    }
}
static inline void regs_add(I2C* i2c, IO* io, uint8_t addr)
{
    int i;
    uint32_t len = karray_size(i2c->regs);
    REG* reg;
    for (i = 0; i < len; i++)
    {
        reg = (REG*)karray_at(i2c->regs, i);
        if (reg->addr == addr)
        {
            kfree(reg->data);
            reg->data = kmalloc(io->data_size);
            memcpy(reg->data, io_data(io), io->data_size);
            reg->len = io->data_size;
            return;
        }
        if (reg->addr > addr)
            break;
    }
    if (i < len)
        reg = (REG*)karray_insert(&i2c->regs, i);
    else
        reg = (REG*)karray_append(&i2c->regs);
    reg->addr = addr;
    reg->data = kmalloc(io->data_size);
    reg->len = io->data_size;
    memcpy(reg->data, io_data(io), io->data_size);
}

static inline REG* regs_search(I2C* i2c, uint8_t addr)
{
    int i;
    REG* reg;
    for (i = 0; i < karray_size(i2c->regs); i++)
    {
        reg = (REG*)karray_at(i2c->regs, i);
        if (reg->addr == addr)
            return reg;
    }
    return NULL;
}

static inline void stm32_i2c_set_register(EXO* exo, IPC* ipc)
{
    I2C_PORT port = (I2C_PORT)ipc->param1;
    I2C* i2c = exo->i2c.i2cs[port];
    IO* io = (IO*)ipc->param2;
    if (i2c == NULL)
    {
        error (ERROR_NOT_CONFIGURED);
        return;
    }
    if (io == NULL)
        regs_delete(i2c, ipc->param3);
    else
        regs_add(i2c, io, ipc->param3);
}

#ifdef STM32F1
static void stm32_i2c_end_rx_slave(I2C* i2c, I2C_PORT port, int error)
{
    if (i2c->io == NULL)
        return;
    else
    {
        I2C_SLAVE_STACK* stack;
        stack = io_push(i2c->io, sizeof(I2C_SLAVE_STACK));
        stack->addr = i2c->reg_addr;
        iio_complete_ex(i2c->process, HAL_IO_CMD(HAL_I2C, IPC_READ), port, i2c->io, error);
        i2c->io = NULL;

    }
}

static inline void stm32_i2c_end_rx(I2C* i2c, I2C_PORT port)
{
    iio_complete(i2c->process, HAL_IO_CMD(HAL_I2C, IPC_READ), port, i2c->io);
    i2c->io_mode = I2C_IO_MODE_IDLE;
    i2c->io = NULL;
    __I2C_REGS[port]->CR1 &= ~(I2C_CR1_ACK | I2C_CR1_POS);
}

static void stm32_i2c_on_error_isr(I2C* i2c, I2C_PORT port, int error, uint32_t sr)
{
    if (sr & I2C_SR1_AF)
    {
        __I2C_REGS[port]->CR1 |= I2C_CR1_STOP;
        error = ERROR_NAK;
    }
    else if (sr & I2C_SR1_TIMEOUT)
    {
        error = ERROR_TIMEOUT;
    }
    else
    {
        error = ERROR_HARDWARE;
    }
    if (i2c->io_mode == I2C_IO_MODE_SLAVE)
    {
        stm32_i2c_end_rx_slave(i2c, port, error);
        return;
    }
    iio_complete_ex(i2c->process, HAL_IO_CMD(HAL_I2C, (i2c->io_mode == I2C_IO_MODE_TX) ? IPC_WRITE : IPC_READ), port, i2c->io, error);
    i2c->io_mode = I2C_IO_MODE_IDLE;
}

static inline void stm32_i2c_on_rx_isr(I2C* i2c, I2C_PORT port, uint32_t sr)
{
    if (sr & I2C_SR1_SB)
    {
        if (i2c->stack->flags & I2C_FLAG_ADDR)
            __I2C_REGS[port]->DR = 0 | (i2c->stack->sla << 1);
        else
        {
            __I2C_REGS[port]->DR = 1 | (i2c->stack->sla << 1);
            if (i2c->size >= 2)
                __I2C_REGS[port]->CR1 |= I2C_CR1_ACK;
            if (i2c->size == 2)
                __I2C_REGS[port]->CR1 |= I2C_CR1_POS;
        }
        return;
    }

    if (sr & I2C_SR1_ADDR)
    {
        __disable_irq();// errata 2.13.1
        __I2C_REGS[port]->SR2;
        if (i2c->stack->flags & I2C_FLAG_ADDR)
        {
            __I2C_REGS[port]->DR = i2c->stack->addr;
            i2c->stack->flags &= ~I2C_FLAG_ADDR;
            __I2C_REGS[port]->CR1 |= I2C_CR1_START;
        }
        else
        {
            if (i2c->size == 1)
            {
                __I2C_REGS[port]->CR1 &= ~I2C_CR1_ACK;
                __I2C_REGS[port]->CR1 |= I2C_CR1_STOP;
            }
            else if (i2c->size == 2)
                __I2C_REGS[port]->CR1 &= ~I2C_CR1_ACK;
        }
        __enable_irq();
        return;
    }
    if (sr & I2C_SR1_TXE)
        return;

    if (sr & I2C_SR1_BTF)
    {
        if (i2c->size == 2)
        {
            __disable_irq(); // errata 2.13.2
            __I2C_REGS[port]->CR1 |= I2C_CR1_STOP;
            ((uint8_t*)io_data(i2c->io))[i2c->io->data_size++] = __I2C_REGS[port]->DR;
            ((uint8_t*)io_data(i2c->io))[i2c->io->data_size++] = __I2C_REGS[port]->DR;
            __enable_irq();
            stm32_i2c_end_rx(i2c, port);
            return;
        }
        else if ((i2c->size - i2c->io->data_size) == 3)
        {
            __I2C_REGS[port]->CR1 &= ~I2C_CR1_ACK;
            __disable_irq();    // errata 2.13.2
            ((uint8_t*)io_data(i2c->io))[i2c->io->data_size++] = __I2C_REGS[port]->DR;
            __I2C_REGS[port]->CR1 |= I2C_CR1_STOP;
            __enable_irq();
            return;
        }
    }

    if (sr & I2C_SR1_RXNE)
    {
        if ((i2c->size > 2) && ((i2c->size - i2c->io->data_size) == 3))
            return;
        if (i2c->size == 2)
            return;
        ((uint8_t*)io_data(i2c->io))[i2c->io->data_size++] = __I2C_REGS[port]->DR;
        if (i2c->io->data_size >= i2c->size)
            stm32_i2c_end_rx(i2c, port);
        return;
    }
}

static inline void stm32_i2c_on_tx_isr(I2C* i2c, I2C_PORT port, uint32_t sr)
{
    if (sr & I2C_SR1_SB)
    {
        __I2C_REGS[port]->DR = i2c->stack->sla << 1;
        return;
    }

    if (sr & I2C_SR1_ADDR)
    {
        __I2C_REGS[port]->SR2;
        if (i2c->stack->flags & I2C_FLAG_ADDR)
            __I2C_REGS[port]->DR = i2c->stack->addr;
        else
            __I2C_REGS[port]->DR = ((uint8_t*)io_data(i2c->io))[i2c->size++];
        return;
    }

    if (sr & I2C_SR1_BTF)
    {
        if (i2c->size >= i2c->io->data_size)
        {
            __I2C_REGS[port]->CR1 |= I2C_CR1_STOP;
            iio_complete(i2c->process, HAL_IO_CMD(HAL_I2C, IPC_WRITE), port, i2c->io);
            i2c->io_mode = I2C_IO_MODE_IDLE;
        }
        else
        {
            __I2C_REGS[port]->DR = ((uint8_t*)io_data(i2c->io))[i2c->size++];
        }
        return;
    }
}

static inline void stm32_i2c_on_slave_isr(I2C* i2c, I2C_PORT port, uint32_t sr)
{
    uint8_t data;
    __I2C_REGS[port]->SR2;
    if (sr & I2C_SR1_TXE)   // slave -> host
    {
        i2c->state = I2C_STATE_DATA;
        if (i2c->reg_data == NULL)
        {
            __I2C_REGS[port]->DR = I2C_REG_EMPTY;
        }
        else
        {
            __I2C_REGS[port]->DR = *i2c->reg_data++;
            if (--i2c->reg_len == 0)
                i2c->reg_data = NULL;
        }
    }

    if (sr & I2C_SR1_RXNE) // host -> slave
    {
        data = __I2C_REGS[port]->DR;
        if (i2c->state == I2C_STATE_ADDR)
        {
            REG* reg = regs_search(i2c, data);
            i2c->reg_addr = data;
            if (reg == NULL)
            {
                i2c->reg_data = NULL;
            }
            else
            {
                i2c->reg_data = reg->data;
                i2c->reg_len = reg->len;
            }
            i2c->state = I2C_STATE_DATA;
        }
        else
        {
            if (i2c->io == NULL)
            {
                __I2C_REGS[port]->CR1 &= ~I2C_CR1_ACK;
            }
            else
            {
                ((uint8_t*)io_data(i2c->io))[i2c->io->data_size++] = data;
                if ((i2c->size - i2c->io->data_size) <= 1)
                    __I2C_REGS[port]->CR1 &= ~I2C_CR1_ACK;
            }
        }
    }

    if (sr & I2C_SR1_ADDR)
    {
        i2c->state = I2C_STATE_ADDR;
    }

    if (sr & I2C_SR1_AF)
    {
        __I2C_REGS[port]->SR1 &= ~I2C_SR1_AF;
    }

    if (sr & I2C_SR1_STOPF)
    {
//        __I2C_REGS[port]->CR1 = __I2C_REGS[port]->CR1;
        __I2C_REGS[port]->CR1 |= I2C_CR1_ACK;
        stm32_i2c_end_rx_slave(i2c, port, 0);
    }
}

// errata 2.13.7
static inline bool check_line(I2C* i2c, bool is_high)
{
    return true;
}

static inline bool clear_busy(EXO* exo, I2C* i2c, I2C_PORT port)
{
    uint32_t cr2, ccr, trise, oar1;
    cr2 = __I2C_REGS[port]->CR2;
    ccr = __I2C_REGS[port]->CCR;
    trise = __I2C_REGS[port]->TRISE;
    oar1 = __I2C_REGS[port]->OAR1;

    __I2C_REGS[port]->CR1 &= ~I2C_CR1_PE;
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_CLOSE), i2c->pin_scl, 0, 0);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_CLOSE), i2c->pin_sda, 0, 0);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_OPEN), i2c->pin_scl, STM32_GPIO_MODE_OUTPUT_OPEN_DRAIN_10MHZ, true);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_OPEN), i2c->pin_sda, STM32_GPIO_MODE_OUTPUT_OPEN_DRAIN_10MHZ, true);
    if (!check_line(i2c, true))
        return false;
    gpio_reset_pin(i2c->pin_sda);
    gpio_reset_pin(i2c->pin_scl);
    if (!check_line(i2c, true))
        return false;
    gpio_set_pin(i2c->pin_scl);
    gpio_set_pin(i2c->pin_sda);
    if (!check_line(i2c, true))
        return false;
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_CLOSE), i2c->pin_scl, 0, 0);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_CLOSE), i2c->pin_sda, 0, 0);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_OPEN), i2c->pin_scl, STM32_GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_10MHZ, true);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_OPEN), i2c->pin_sda, STM32_GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_10MHZ, true);
    __I2C_REGS[port]->CR1 |= I2C_CR1_SWRST;
    __I2C_REGS[port]->CR1 &= ~I2C_CR1_SWRST;
    __I2C_REGS[port]->CR2 = cr2;
    __I2C_REGS[port]->CCR = ccr;
    __I2C_REGS[port]->TRISE = trise;
    __I2C_REGS[port]->OAR1 = oar1;

    __I2C_REGS[port]->CR1 |= I2C_CR1_PE;
    if (i2c->io_mode == I2C_IO_MODE_SLAVE)
        __I2C_REGS[port]->CR1 |= I2C_CR1_ACK;
    return true;
}

#else // STM32F1

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

static inline void stm32_i2c_on_rx_isr(I2C* i2c, I2C_PORT port, uint32_t sr)
{
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
    default:
        break;
    }
}

static inline void stm32_i2c_on_tx_isr(I2C* i2c, I2C_PORT port, uint32_t sr)
{
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
            stm32_i2c_on_error_isr(i2c, port, ERROR_NAK);
        break;
    default:
        break;
    }
}

static inline void stm32_i2c_on_slave_isr(I2C* i2c, I2C_PORT port, uint32_t sr)
{
    // TODO: add slave to stm32f0..
}

#endif  //STM32F1

void stm32_i2c_on_isr(int vector, void* param)
{
    I2C_PORT port;
    I2C* i2c;
    EXO* exo = param;
    uint32_t sr;
    port = I2C_1;
#if (I2C_COUNT > 1)
#ifdef STM32F1
    if ((vector == __I2C_VECTORS[1]) || (vector == __I2C_ERROR_VECTORS[1]))
#else
    if (vector != __I2C_VECTORS[0])
#endif
    port = I2C_2;
#endif
    i2c = exo->i2c.i2cs[port];
#ifdef STM32F1
    sr = __I2C_REGS[port]->SR1;
    if (sr & I2C_SR_ERROR_MASK)
    {
        stm32_i2c_on_error_isr(i2c, port, 0, sr);
        __I2C_REGS[port]->SR1 &= ~I2C_SR_ERROR_MASK;
        return;
    }
#else //STM32F1
    sr = __I2C_REGS[port]->ISR;
    if (sr & (I2C_ISR_ARLO | I2C_ISR_BERR))
    {
        stm32_i2c_on_error_isr(i2c, port, ERROR_HARDWARE);
        return;
    }
#endif //STM32F1

    switch (i2c->io_mode)
    {
    case I2C_IO_MODE_TX:
        stm32_i2c_on_tx_isr(i2c, port, sr);
        break;
    case I2C_IO_MODE_RX:
        stm32_i2c_on_rx_isr(i2c, port, sr);
        break;
    case I2C_IO_MODE_SLAVE:
        stm32_i2c_on_slave_isr(i2c, port, sr);
        break;
    default:
        #ifdef STM32F1
        stm32_i2c_on_error_isr(i2c, port, ERROR_INVALID_STATE, sr);
#else
        stm32_i2c_on_error_isr(i2c, port, ERROR_INVALID_STATE);
#endif //STM32F1
    }
}

void stm32_i2c_init(EXO* exo)
{
    int i;
    for (i = 0; i < I2C_COUNT; ++i)
        exo->i2c.i2cs[i] = NULL;
}

void stm32_i2c_open(EXO* exo, I2C_PORT port, unsigned int mode, unsigned int speed)
{
    I2C* i2c = exo->i2c.i2cs[port];
    if (i2c)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    i2c = kmalloc(sizeof(I2C));
    exo->i2c.i2cs[port] = i2c;
    if (i2c == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
    i2c->io = NULL;
    i2c->io_mode = I2C_IO_MODE_IDLE;
    i2c->regs = NULL;

    //enable clock
    RCC->APB1ENR |= (1 << __I2C_POWER_PINS[port]);
#if defined(STM32F1)
    i2c->pin_scl = (mode & STM32F1_I2C_SCL_Msk) >> STM32F1_I2C_SCL_Pos;
    i2c->pin_sda = (mode & STM32F1_I2C_SDA_Msk) >> STM32F1_I2C_SDA_Pos;
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_OPEN), i2c->pin_scl, STM32_GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_10MHZ, true);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_OPEN), i2c->pin_sda, STM32_GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_10MHZ, true);
    uint32_t arb1_freq = stm32_power_get_clock_inside(exo, STM32_CLOCK_APB1);
    __I2C_REGS[port]->CR2 = I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | (arb1_freq / 1000000);
    if (speed <= I2C_NORMAL_CLOCK)
    {
        __I2C_REGS[port]->CCR = (arb1_freq / (2 * 88000)) + 1; // speed must not be from 88 to 100KHz, errata 2.13.5
        __I2C_REGS[port]->TRISE = 1 + arb1_freq / 1000000;// 1000 ns

    }
    else
    {
        __I2C_REGS[port]->CCR = I2C_CCR_FS | (arb1_freq / 800000);
        __I2C_REGS[port]->TRISE = 1 + arb1_freq / 3000000; // 300 ns
    }
    if ((mode & I2C_MODE) == I2C_MODE_SLAVE)
    {
        regs_init(i2c);
        __I2C_REGS[port]->OAR1 = (mode & I2C_SLAVE_ADDR) << 1;
        i2c->io_mode = I2C_IO_MODE_SLAVE;
        __I2C_REGS[port]->CR1 = I2C_CR1_PE | I2C_CR1_ACK;
    }
    else
    {
        __I2C_REGS[port]->CR1 = I2C_CR1_PE;
    }

    if (__I2C_REGS[port]->SR2 & I2C_SR2_BUSY)
    clear_busy(exo, i2c, port);

    kirq_register(KERNEL_HANDLE, __I2C_ERROR_VECTORS[port], stm32_i2c_on_isr, (void*)exo);
    NVIC_EnableIRQ(__I2C_ERROR_VECTORS[port]);
    NVIC_SetPriority(__I2C_ERROR_VECTORS[port], 13);

#else //STM32F1
    if (speed <= I2C_NORMAL_CLOCK)
        __I2C_REGS[port]->TIMINGR = I2C_TIMING_NORMAL_SPEED;
    else if (speed <= I2C_FAST_CLOCK)
        __I2C_REGS[port]->TIMINGR = I2C_TIMING_FAST_SPEED;
    else
        __I2C_REGS[port]->TIMINGR = I2C_TIMING_FAST_PLUS_SPEED;

    __I2C_REGS[port]->CR1 |= I2C_CR1_PE | I2C_CR1_ERRIE | I2C_CR1_TCIE | I2C_CR1_NACKIE | I2C_CR1_RXIE | I2C_CR1_TXIE;
#endif //STM32F1
    //enable interrupt
    kirq_register(KERNEL_HANDLE, __I2C_VECTORS[port], stm32_i2c_on_isr, (void*)exo);
    NVIC_EnableIRQ(__I2C_VECTORS[port]);
    NVIC_SetPriority(__I2C_VECTORS[port], 13);
}

void stm32_i2c_close(EXO* exo, I2C_PORT port)
{
    I2C* i2c = exo->i2c.i2cs[port];
    if (i2c == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
#if defined(STM32F1)
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_CLOSE), i2c->pin_scl, 0, 0);
    stm32_pin_request_inside(exo, HAL_CMD(HAL_PIN, IPC_CLOSE), i2c->pin_sda, 0, 0);
    NVIC_DisableIRQ(__I2C_ERROR_VECTORS[port]);
    kirq_unregister(KERNEL_HANDLE, __I2C_ERROR_VECTORS[port]);
#endif
    NVIC_DisableIRQ(__I2C_VECTORS[port]);
    kirq_unregister(KERNEL_HANDLE, __I2C_VECTORS[port]);
    __I2C_REGS[port]->CR1 &= ~I2C_CR1_PE;
    RCC->APB1ENR &= ~(1 << __I2C_POWER_PINS[port]);

    regs_free(i2c);
    kfree(i2c);
    exo->i2c.i2cs[port] = NULL;
}

static void stm32_i2c_io(EXO* exo, IPC* ipc, bool read)
{
    I2C_PORT port = (I2C_PORT)ipc->param1;
    I2C* i2c = exo->i2c.i2cs[port];
    if (i2c == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if ((i2c->io_mode != I2C_IO_MODE_IDLE) && (i2c->io_mode != I2C_IO_MODE_SLAVE))
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    i2c->process = ipc->process;
    i2c->io = (IO*)ipc->param2;
    i2c->stack = io_stack(i2c->io);
    if (i2c->io_mode == I2C_IO_MODE_SLAVE)
    {
        i2c->io->data_size = 0;
        i2c->size = ipc->param3;
        io_reset(i2c->io);
        error (ERROR_SYNC);
        return;
    }

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
#if defined(STM32F1)
    if (__I2C_REGS[port]->SR2 & I2C_SR2_BUSY)
    clear_busy(exo, i2c, port);
    i2c->state = I2C_STATE_DATA;
    if (i2c->stack->flags & I2C_FLAG_LEN)
    i2c->state = I2C_STATE_LEN;
    if (i2c->stack->flags & I2C_STATE_ADDR)
    i2c->state = I2C_STATE_ADDR;

    //set START
    __I2C_REGS[port]->CR1 |= I2C_CR1_START;
    //all rest in isr
#else //STM32F1
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
#endif //STM32F1

    error (ERROR_SYNC);
}

void stm32_i2c_request(EXO* exo, IPC* ipc)
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
        stm32_i2c_open(exo, port, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        stm32_i2c_close(exo, port);
        break;
    case IPC_WRITE:
        stm32_i2c_io(exo, ipc, false);
        break;
    case IPC_READ:
        stm32_i2c_io(exo, ipc, true);
        break;
    case I2C_SET_REGISTER:
        stm32_i2c_set_register(exo, ipc);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
