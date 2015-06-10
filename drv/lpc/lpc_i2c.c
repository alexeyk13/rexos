/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_i2c.h"
#include "lpc_gpio.h"
#include "lpc_core_private.h"
#include "lpc_power.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/irq.h"
#include "../../userspace/timer.h"

#define get_system_clock                            lpc_power_get_system_clock_inside
#define ack_gpio                                    lpc_gpio_request_inside

#define I2C_NORMAL_CLOCK                            100000
#define I2C_FAST_CLOCK                              400000

#define ADDR_SIZE(core, port)                        (((core)->i2c.i2cs[(port)]->mode & I2C_ADDR_SIZE_MASK) >> I2C_ADDR_SIZE_POS)
#define ADDR_BYTE(core, port)                        (((core)->i2c.i2cs[(port)]->addr >> ((((core)->i2c.i2cs[(port)]->mode & I2C_ADDR_SIZE_MASK) - \
                                                    ++(core)->i2c.i2cs[(port)]->addr_processed) << 3)) & 0xff)

#define RX_LEN_SIZE(core, port)                      (((core)->i2c.i2cs[(port)]->mode & I2C_LEN_SIZE_MASK) >> I2C_LEN_SIZE_POS)
#define RX_LEN_SET_BYTE(core, port, byte)            ((core)->i2c.i2cs[(port)]->rx_len = (((core)->i2c.i2cs[(port)]->rx_len) << 8) | ((byte) & 0xff))

#define I2C_SCL_PIN                                 PIO0_4
#define I2C_SDA_PIN                                 PIO0_5
#define I2C_VECTOR                                  15

#define I2C_CLEAR (I2C_CONCLR_AAC | I2C_CONCLR_SIC | I2C_CONCLR_STOC | I2C_CONCLR_STAC)

void lpc_i2c_isr_error(CORE* core, I2C_PORT port, int error)
{
    I2C* i2c = core->i2c.i2cs[port];
    LPC_I2C->CONSET = I2C_CONSET_STO;
#if (LPC_I2C_TIMEOUT_MS)
    timer_istop(i2c->timer);
#endif
    iio_complete_ex(i2c->process, HAL_CMD(HAL_I2C, (i2c->io_mode == I2C_IO_MODE_TX) ? IPC_WRITE : IPC_READ), port, i2c->io, error);
    i2c->io_mode = I2C_IO_MODE_IDLE;
}

static inline void lpc_i2c_isr_tx(CORE* core, I2C_PORT port)
{
    I2C* i2c = core->i2c.i2cs[port];
    switch(LPC_I2C->STAT)
    {
    case I2C_STAT_START:
        //transmit address
        LPC_I2C->DAT = (i2c->sla << 1) | 0;
        LPC_I2C->CONCLR = I2C_CONCLR_STAC;
        break;
    case I2C_STAT_SLAW_NACK:
        lpc_i2c_isr_error(core, port, ERROR_NAK);
        break;
    case I2C_STAT_DATW_NACK:
        //only acceptable for last byte
        if (i2c->size < i2c->io->data_size || i2c->addr_processed < ADDR_SIZE(core, port))
        {
            lpc_i2c_isr_error(core, port, ERROR_NAK);
            break;
        }
        //follow down
    case I2C_STAT_SLAW_ACK:
    case I2C_STAT_DATW_ACK:
        if (i2c->addr_processed < ADDR_SIZE(core, port))
            LPC_I2C->DAT = ADDR_BYTE(core, port);
        else if (i2c->size < i2c->io->data_size)
        {
            LPC_I2C->DAT = *((uint8_t*)(io_data(i2c->io) + i2c->io->data_size));
            ++i2c->io->data_size;
        }
        //last byte
        else
        {
            LPC_I2C->CONSET = I2C_CONSET_STO;
#if (LPC_I2C_TIMEOUT_MS)
            timer_istop(i2c->timer);
#endif
            iio_complete(i2c->process, HAL_CMD(HAL_I2C, IPC_WRITE), port, i2c->io);
            i2c->io_mode = I2C_IO_MODE_IDLE;
        }
        break;
    default:
         lpc_i2c_isr_error(core, port, ERROR_INVALID_STATE);
         break;
    }
}

static inline void lpc_i2c_isr_rx(CORE* core, I2C_PORT port)
{
    I2C* i2c = core->i2c.i2cs[port];
    switch(LPC_I2C->STAT)
    {
    case I2C_STAT_START:
        //transmit address W
        if (ADDR_SIZE(core, port))
        {
            LPC_I2C->DAT = (i2c->sla << 1) | 0;
            LPC_I2C->CONCLR = I2C_CONCLR_STAC;
            break;
        }
    case I2C_STAT_REPEATED_START:
        //transmit address R
        LPC_I2C->CONCLR = I2C_CONCLR_STAC;
        LPC_I2C->DAT = (i2c->sla << 1) | 1;
        break;
    case I2C_STAT_SLAR_NACK:
    case I2C_STAT_SLAW_NACK:
        lpc_i2c_isr_error(core, port, ERROR_NAK);
        break;
    case I2C_STAT_DATW_NACK:
        //only acceptable for last W byte
        if (i2c->addr_processed < ADDR_SIZE(core, port))
        {
            lpc_i2c_isr_error(core, port, ERROR_NAK);
            break;
        }
    case I2C_STAT_SLAW_ACK:
    case I2C_STAT_DATW_ACK:
        if (i2c->addr_processed < ADDR_SIZE(core, port))
            LPC_I2C->DAT = ADDR_BYTE(core, port);
        //last addr byte, rS
        else
            LPC_I2C->CONSET = I2C_CONSET_STA;
        break;
    case I2C_STAT_SLAR_ACK:
        LPC_I2C->CONSET = I2C_CONSET_AA;
        break;
    case I2C_STAT_DATR_ACK:
        if (i2c->rx_len_processed < RX_LEN_SIZE(core, port))
        {
            RX_LEN_SET_BYTE(core, port, LPC_I2C->DAT);
            if (++(i2c->rx_len_processed) >= RX_LEN_SIZE(core, port))
            {
                if (i2c->size > i2c->rx_len)
                    i2c->size = i2c->rx_len;
            }
        }
        else
            *((uint8_t*)(io_data(i2c->io) + i2c->io->data_size)) = LPC_I2C->DAT;
            ++i2c->io->data_size;
        //need more? send ACK
        if (i2c->io->data_size + 1 < i2c->size)
            LPC_I2C->CONSET = I2C_CONSET_AA;
        //received all - send NAK
        else
            LPC_I2C->CONCLR = I2C_CONCLR_AAC;
        break;
    case I2C_STAT_DATR_NACK:
        //last byte
        *((uint8_t*)(io_data(i2c->io) + i2c->io->data_size)) = LPC_I2C->DAT;
        ++i2c->io->data_size;
        //stop transmission
        LPC_I2C->CONSET = I2C_CONSET_STO;
#if (LPC_I2C_TIMEOUT_MS)
        timer_istop(i2c->timer);
#endif
        iio_complete(i2c->process, HAL_CMD(HAL_I2C, IPC_READ), port, i2c->io);
        i2c->io_mode = I2C_IO_MODE_IDLE;
        break;
    default:
        lpc_i2c_isr_error(core, port, ERROR_INVALID_STATE);
        break;
    }
}

void lpc_i2c_on_isr(int vector, void* param)
{
    CORE* core = (CORE*)param;
    switch (core->i2c.i2cs[I2C_0]->io_mode)
    {
    case I2C_IO_MODE_TX:
        lpc_i2c_isr_tx(core, I2C_0);
        break;
    case I2C_IO_MODE_RX:
        lpc_i2c_isr_rx(core, I2C_0);
        break;
    default:
        break;
    }
    LPC_I2C->CONCLR = I2C_CONCLR_SIC;
}

void lpc_i2c_open(CORE* core, I2C_PORT port, unsigned int mode, unsigned int sla)
{
    I2C* i2c = core->i2c.i2cs[port];
    if (i2c)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    i2c = malloc(sizeof(I2C));
    if (i2c == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
#if (LPC_I2C_TIMEOUT_MS)
    i2c->timer = timer_create(port, HAL_I2C);
    if (i2c->timer == INVALID_HANDLE)
    {
        free(i2c);
        i2c = NULL;
        return;
    }
#endif
    i2c->sla = sla & 0x7f;
    i2c->mode = mode;
    i2c->io = NULL;
    i2c->io_mode = I2C_IO_MODE_IDLE;
    i2c->addr = 0;
    //setup pins
    ack_gpio(core, HAL_CMD(HAL_GPIO, LPC_GPIO_ENABLE_PIN), I2C_SCL_PIN, PIN_MODE_I2C_SCL | (mode & I2C_FAST_SPEED ? GPIO_I2C_MODE_FAST : GPIO_I2C_MODE_STANDART), 0);
    ack_gpio(core, HAL_CMD(HAL_GPIO, LPC_GPIO_ENABLE_PIN), I2C_SDA_PIN, PIN_MODE_I2C_SDA | (mode & I2C_FAST_SPEED ? GPIO_I2C_MODE_FAST : GPIO_I2C_MODE_STANDART), 0);
    //power up
    LPC_SYSCON->SYSAHBCLKCTRL |= 1 << SYSCON_SYSAHBCLKCTRL_I2C0_POS;
    //remove reset state
    LPC_SYSCON->PRESETCTRL |= 1 << SYSCON_PRESETCTRL_I2C0_RST_N_POS;
    //setup clock
    LPC_I2C->SCLL = LPC_I2C->SCLH = get_system_clock(core) / (mode & I2C_FAST_SPEED ? I2C_FAST_CLOCK : I2C_NORMAL_CLOCK) / 2;
    //reset state machine
    LPC_I2C->CONCLR = I2C_CLEAR;
    //enable interrupt
    irq_register(I2C_VECTOR, lpc_i2c_on_isr, (void*)core);
    NVIC_EnableIRQ(I2C_VECTOR);
    NVIC_SetPriority(I2C_VECTOR, 2);
}

void lpc_i2c_close(CORE* core, I2C_PORT port)
{
    I2C* i2c = core->i2c.i2cs[port];
    if (i2c == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //disable interrupt
    NVIC_DisableIRQ(I2C_VECTOR);
    irq_unregister(I2C_VECTOR);

    //set reset state
    LPC_SYSCON->PRESETCTRL &= ~(1 << SYSCON_PRESETCTRL_I2C0_RST_N_POS);
    //power down
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << SYSCON_SYSAHBCLKCTRL_I2C0_POS);
    //disable pins
    ack_gpio(core, HAL_CMD(HAL_GPIO, LPC_GPIO_DISABLE_PIN), I2C_SCL_PIN, 0, 0);
    ack_gpio(core, HAL_CMD(HAL_GPIO, LPC_GPIO_DISABLE_PIN), I2C_SDA_PIN, 0, 0);
}

static inline void lpc_i2c_read(CORE* core, IPC* ipc)
{
    I2C_PORT port = (I2C_PORT)ipc->param1;
    I2C* i2c = core->i2c.i2cs[port];
    if (i2c == NULL)
    {
        io_send_error(ipc, ERROR_NOT_CONFIGURED);
        return;
    }
    if (i2c->io_mode != I2C_IO_MODE_IDLE)
    {
        io_send_error(ipc, ERROR_IN_PROGRESS);
        return;
    }
    i2c->process = ipc->process;
    i2c->io = (IO*)ipc->param2;
    i2c->io->data_size = 0;
    i2c->size = ipc->param3;
    i2c->addr_processed = 0;
    i2c->rx_len_processed = 0;
    i2c->rx_len = 0;
    i2c->io_mode = I2C_IO_MODE_RX;

    //reset
    LPC_I2C->CONCLR = I2C_CLEAR;
#if (LPC_I2C_TIMEOUT_MS)
    timer_start_ms(i2c->timer, LPC_I2C_TIMEOUT_MS, 0);
#endif

    //set START
    LPC_I2C->CONSET = I2C_CONSET_I2EN | I2C_CONSET_STA;
    //all rest in isr
}

static inline void lpc_i2c_write(CORE* core, IPC* ipc)
{
    I2C_PORT port = (I2C_PORT)ipc->param1;
    I2C* i2c = core->i2c.i2cs[port];
    if (i2c == NULL)
    {
        io_send_error(ipc, ERROR_NOT_CONFIGURED);
        return;
    }
    if (i2c->io_mode != I2C_IO_MODE_IDLE)
    {
        io_send_error(ipc, ERROR_IN_PROGRESS);
        return;
    }
    i2c->process = ipc->process;
    i2c->io = (IO*)ipc->param2;
    i2c->size = 0;
    i2c->addr_processed = 0;
    i2c->rx_len_processed = 0;
    i2c->rx_len = 0;
    i2c->io_mode = I2C_IO_MODE_TX;

    //reset
    LPC_I2C->CONCLR = I2C_CLEAR;
#if (LPC_I2C_TIMEOUT_MS)
    timer_start_ms(i2c->timer, LPC_I2C_TIMEOUT_MS, 0);
#endif
    //set START
    LPC_I2C->CONSET = I2C_CONSET_I2EN | I2C_CONSET_STA;
    //all rest in isr
}

static inline void lpc_i2c_seek(CORE* core, I2C_PORT port, unsigned int pos)
{
    core->i2c.i2cs[port]->addr = pos;
}

#if (LPC_I2C_TIMEOUT_MS)
static inline void lpc_i2c_timeout(CORE* core, I2C_PORT port)
{
    I2C_IO_MODE io_mode;
    I2C* i2c = core->i2c.i2cs[port];
    __disable_irq();
    io_mode = i2c->io_mode;
    i2c->io_mode = I2C_IO_MODE_IDLE;
    __enable_irq();
    //handled right now in isr
    if (io_mode == I2C_IO_MODE_IDLE)
        return;
    LPC_I2C->CONSET = I2C_CONSET_STO;
    io_complete_ex(i2c->process, HAL_CMD(HAL_I2C, (io_mode == I2C_IO_MODE_TX) ? IPC_WRITE : IPC_READ), port, i2c->io, ERROR_TIMEOUT);
}
#endif

void lpc_i2c_init(CORE* core)
{
    int i;
    for (i = 0; i < I2C_COUNT; ++i)
        core->i2c.i2cs[i] = NULL;
}

bool lpc_i2c_request(CORE* core, IPC* ipc)
{
    I2C_PORT port = (I2C_PORT)ipc->param1;
    if (port >= I2C_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return true;
    }
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_i2c_open(core, port, ipc->param2, ipc->param3);
        need_post = true;
        break;
    case IPC_CLOSE:
        lpc_i2c_close(core, port);
        need_post = true;
        break;
    case IPC_WRITE:
        lpc_i2c_write(core, ipc);
        //async message, no write
        break;
    case IPC_READ:
        lpc_i2c_read(core, ipc);
        //async message, no write
        break;
    case IPC_SEEK:
        lpc_i2c_seek(core, port, ipc->param2);
        need_post = true;
        break;
#if (LPC_I2C_TIMEOUT_MS)
    case IPC_TIMEOUT:
        lpc_i2c_timeout(core, port);
        break;
#endif
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
