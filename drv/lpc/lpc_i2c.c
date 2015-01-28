/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_i2c.h"
#include "lpc_gpio.h"
#include "lpc_core.h"
#include "lpc_power.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/irq.h"
#include "../../userspace/block.h"
#include "../../userspace/file.h"
#if (SYS_INFO)
#include "../../userspace/stdio.h"
#endif

#if (MONOLITH_I2C)
#include "lpc_core_private.h"


#define get_system_clock        lpc_power_get_system_clock_inside
#define ack_gpio                lpc_gpio_request_inside

#else

#define get_system_clock        lpc_power_get_system_clock_outside
#define ack_gpio                lpc_core_request_outside

void lpc_i2c();

const REX __LPC_I2C = {
    //name
    "LPC I2C",
    //size
    LPC_I2C_PROCESS_SIZE,
    //priority - driver priority. Setting priority lower than other drivers can cause IPC overflow on SYS_INFO
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    LPC_DRIVERS_IPC_COUNT,
    //function
    lpc_i2c
};
#endif

#define I2C_NORMAL_CLOCK                            100000
#define I2C_FAST_CLOCK                              400000

#define ADDR_SIZE(drv, port)                        (((drv)->i2c.i2cs[(port)]->mode & I2C_ADDR_SIZE_MASK) >> I2C_ADDR_SIZE_POS)
#define ADDR_BYTE(drv, port)                        (((drv)->i2c.i2cs[(port)]->addr >> ((((drv)->i2c.i2cs[(port)]->mode & I2C_ADDR_SIZE_MASK) - \
                                                    ++(drv)->i2c.i2cs[(port)]->addr_processed) << 3)) & 0xff)

#define RX_LEN_SIZE(drv, port)                      (((drv)->i2c.i2cs[(port)]->mode & I2C_LEN_SIZE_MASK) >> I2C_LEN_SIZE_POS)
#define RX_LEN_SET_BYTE(drv, port, byte)            ((drv)->i2c.i2cs[(port)]->rx_len = (((drv)->i2c.i2cs[(port)]->rx_len) << 8) | ((byte) & 0xff))

static const PIN __I2C_SCL[] =                      {PIO0_4};
static const PIN __I2C_SDA[] =                      {PIO0_5};
static const uint8_t __I2C_VECTORS[] =              {15};

#define I2C_CLEAR (I2C_CONCLR_AAC | I2C_CONCLR_SIC | I2C_CONCLR_STOC | I2C_CONCLR_STAC)

void lpc_i2c_isr_error(SHARED_I2C_DRV* drv, I2C_PORT port, int error)
{
    IPC ipc;
    LPC_I2C->CONSET = I2C_CONSET_STO;
    switch (drv->i2c.i2cs[I2C_0]->io)
    {
    case I2C_IO_TX:
        ipc.cmd = IPC_WRITE_COMPLETE;
        break;
    default:
        ipc.cmd = IPC_READ_COMPLETE;
        break;
    }
    ipc.process = drv->i2c.i2cs[port]->process;
    ipc.param1 = HAL_HANDLE(HAL_I2C, port);
    ipc.param2 = drv->i2c.i2cs[port]->block;
    ipc.param3 = error;
    if (drv->i2c.i2cs[port]->block != INVALID_HANDLE)
        block_isend_ipc(drv->i2c.i2cs[port]->block, drv->i2c.i2cs[port]->process, &ipc);
    else
        ipc_ipost(&ipc);
    drv->i2c.i2cs[port]->io = I2C_IO_IDLE;
}

static inline void lpc_i2c_isr_tx(SHARED_I2C_DRV* drv, I2C_PORT port)
{
    IPC ipc;
     switch(LPC_I2C->STAT)
     {
     case I2C_STAT_START:
         //transmit address
         LPC_I2C->DAT = (drv->i2c.i2cs[port]->sla << 1) | 0;
         LPC_I2C->CONCLR = I2C_CONCLR_STAC;
         break;
     case I2C_STAT_SLAW_NACK:
         lpc_i2c_isr_error(drv, port, ERROR_NAK);
         break;
     case I2C_STAT_DATW_NACK:
         //only acceptable for last byte
         if (drv->i2c.i2cs[port]->processed < drv->i2c.i2cs[port]->size || drv->i2c.i2cs[port]->addr_processed < ADDR_SIZE(drv, port))
         {
             lpc_i2c_isr_error(drv, port, ERROR_NAK);
             break;
         }
     case I2C_STAT_SLAW_ACK:
     case I2C_STAT_DATW_ACK:
         if (drv->i2c.i2cs[port]->addr_processed < ADDR_SIZE(drv, port))
             LPC_I2C->DAT = ADDR_BYTE(drv, port);
         else if (drv->i2c.i2cs[port]->processed < drv->i2c.i2cs[port]->size)
             LPC_I2C->DAT = drv->i2c.i2cs[port]->ptr[drv->i2c.i2cs[port]->processed++];
         //last byte
         else
         {
             LPC_I2C->CONSET = I2C_CONSET_STO;
             if (drv->i2c.i2cs[port]->block != INVALID_HANDLE)
                 block_isend(drv->i2c.i2cs[port]->block, drv->i2c.i2cs[port]->process);
             ipc.process = drv->i2c.i2cs[port]->process;
             ipc.cmd = IPC_WRITE_COMPLETE;
             ipc.param1 = HAL_HANDLE(HAL_I2C, port);
             ipc.param2 = drv->i2c.i2cs[port]->block;
             ipc.param3 = drv->i2c.i2cs[port]->processed;
             ipc_ipost(&ipc);
             drv->i2c.i2cs[port]->io = I2C_IO_IDLE;
         }
         break;
     default:
         lpc_i2c_isr_error(drv, port, ERROR_INVALID_STATE);
         break;
     }
}

static inline void lpc_i2c_isr_rx(SHARED_I2C_DRV* drv, I2C_PORT port)
{
    IPC ipc;
    switch(LPC_I2C->STAT)
    {
    case I2C_STAT_START:
        //transmit address W
        if (ADDR_SIZE(drv, port))
        {
            LPC_I2C->DAT = (drv->i2c.i2cs[port]->sla << 1) | 0;
            LPC_I2C->CONCLR = I2C_CONCLR_STAC;
            break;
        }
    case I2C_STAT_REPEATED_START:
        //transmit address R
        LPC_I2C->CONCLR = I2C_CONCLR_STAC;
        LPC_I2C->DAT = (drv->i2c.i2cs[port]->sla << 1) | 1;
        break;
    case I2C_STAT_SLAR_NACK:
    case I2C_STAT_SLAW_NACK:
        lpc_i2c_isr_error(drv, port, ERROR_NAK);
        break;
    case I2C_STAT_DATW_NACK:
        //only acceptable for last W byte
        if (drv->i2c.i2cs[port]->addr_processed < ADDR_SIZE(drv, port))
        {
            lpc_i2c_isr_error(drv, port, ERROR_NAK);
            break;
        }
    case I2C_STAT_SLAW_ACK:
    case I2C_STAT_DATW_ACK:
        if (drv->i2c.i2cs[port]->addr_processed < ADDR_SIZE(drv, port))
            LPC_I2C->DAT = ADDR_BYTE(drv, port);
        //last addr byte, rS
        else
            LPC_I2C->CONSET = I2C_CONSET_STA;
        break;
    case I2C_STAT_SLAR_ACK:
        LPC_I2C->CONSET = I2C_CONSET_AA;
        break;
    case I2C_STAT_DATR_ACK:
        if (drv->i2c.i2cs[port]->rx_len_processed < RX_LEN_SIZE(drv, port))
        {
            RX_LEN_SET_BYTE(drv, port, LPC_I2C->DAT);
            if (++(drv->i2c.i2cs[port]->rx_len_processed) >= RX_LEN_SIZE(drv, port))
            {
                if (drv->i2c.i2cs[port]->size > drv->i2c.i2cs[port]->rx_len)
                    drv->i2c.i2cs[port]->size = drv->i2c.i2cs[port]->rx_len;
            }
        }
        else
            drv->i2c.i2cs[port]->ptr[drv->i2c.i2cs[port]->processed++] = LPC_I2C->DAT;
        //need more? send ACK
        if (drv->i2c.i2cs[port]->processed + 1 < drv->i2c.i2cs[port]->size)
            LPC_I2C->CONSET = I2C_CONSET_AA;
        //received all - send NAK
        else
            LPC_I2C->CONCLR = I2C_CONCLR_AAC;
        break;
    case I2C_STAT_DATR_NACK:
        //last byte
        drv->i2c.i2cs[port]->ptr[drv->i2c.i2cs[port]->processed++] = LPC_I2C->DAT;
        //stop transmission
        LPC_I2C->CONSET = I2C_CONSET_STO;
        if (drv->i2c.i2cs[port]->block != INVALID_HANDLE)
            block_isend(drv->i2c.i2cs[port]->block, drv->i2c.i2cs[port]->process);
        ipc.process = drv->i2c.i2cs[port]->process;
        ipc.cmd = IPC_READ_COMPLETE;
        ipc.param1 = HAL_HANDLE(HAL_I2C, port);
        ipc.param2 = drv->i2c.i2cs[port]->block;
        ipc.param3 = drv->i2c.i2cs[port]->processed;
        ipc_ipost(&ipc);
        drv->i2c.i2cs[port]->io = I2C_IO_IDLE;
        break;
    default:
        lpc_i2c_isr_error(drv, port, ERROR_INVALID_STATE);
        break;
    }
}

void lpc_i2c_on_isr(int vector, void* param)
{
    SHARED_I2C_DRV* drv = (SHARED_I2C_DRV*)param;
    switch (drv->i2c.i2cs[I2C_0]->io)
    {
    case I2C_IO_TX:
        lpc_i2c_isr_tx(drv, I2C_0);
        break;
    case I2C_IO_RX:
        lpc_i2c_isr_rx(drv, I2C_0);
        break;
    default:
        break;
    }
    LPC_I2C->CONCLR = I2C_CONCLR_SIC;
}

void lpc_i2c_open(SHARED_I2C_DRV *drv, I2C_PORT port, unsigned int mode, unsigned int sla)
{
    if (port >= I2C_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->i2c.i2cs[port])
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    drv->i2c.i2cs[port] = malloc(sizeof(I2C));
    if (drv->i2c.i2cs[port] == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }
    drv->i2c.i2cs[port]->sla = sla & 0x7f;
    drv->i2c.i2cs[port]->mode = mode;
    drv->i2c.i2cs[port]->block = INVALID_HANDLE;
    drv->i2c.i2cs[port]->io = I2C_IO_IDLE;
    drv->i2c.i2cs[port]->addr = 0;
    //setup pins
    ack_gpio(drv, LPC_GPIO_ENABLE_PIN, __I2C_SCL[port], 0, mode & I2C_FAST_SPEED ? AF_FAST_I2C : AF_I2C);
    ack_gpio(drv, LPC_GPIO_ENABLE_PIN, __I2C_SDA[port], 0, mode & I2C_FAST_SPEED ? AF_FAST_I2C : AF_I2C);
    //power up
    LPC_SYSCON->SYSAHBCLKCTRL |= 1 << SYSCON_SYSAHBCLKCTRL_I2C0_POS;
    //remove reset state
    LPC_SYSCON->PRESETCTRL |= 1 << SYSCON_PRESETCTRL_I2C0_RST_N_POS;
    //setup clock
    LPC_I2C->SCLL = LPC_I2C->SCLH = get_system_clock(drv) / (mode & I2C_FAST_SPEED ? I2C_FAST_CLOCK : I2C_NORMAL_CLOCK) / 2;
    //reset state machine
    LPC_I2C->CONCLR = I2C_CLEAR;
    //enable interrupt
    irq_register(__I2C_VECTORS[port], lpc_i2c_on_isr, (void*)drv);
    NVIC_EnableIRQ(__I2C_VECTORS[port]);
    NVIC_SetPriority(__I2C_VECTORS[port], 13);
}

void lpc_i2c_close(SHARED_I2C_DRV *drv, I2C_PORT port)
{
    if (port >= I2C_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->i2c.i2cs[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //disable interrupt
    NVIC_DisableIRQ(__I2C_VECTORS[port]);
    irq_unregister(__I2C_VECTORS[port]);

    //set reset state
    LPC_SYSCON->PRESETCTRL &= ~(1 << SYSCON_PRESETCTRL_I2C0_RST_N_POS);
    //power down
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << SYSCON_SYSAHBCLKCTRL_I2C0_POS);
    //disable pins
    ack_gpio(drv, LPC_GPIO_DISABLE_PIN, __I2C_SCL[port], 0, 0);
    ack_gpio(drv, LPC_GPIO_DISABLE_PIN, __I2C_SDA[port], 0, 0);
}

static inline void lpc_i2c_read(SHARED_I2C_DRV* drv, I2C_PORT port, HANDLE block, unsigned int size, HANDLE process)
{
    if (port >= I2C_COUNT || (size && block == INVALID_HANDLE))
    {
        fread_complete(process, HAL_HANDLE(HAL_I2C, port), block, ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->i2c.i2cs[port] == NULL)
    {
        fread_complete(process, HAL_HANDLE(HAL_I2C, port), block, ERROR_NOT_CONFIGURED);
        return;
    }
    if (drv->i2c.i2cs[port]->io != I2C_IO_IDLE)
    {
        fread_complete(process, HAL_HANDLE(HAL_I2C, port), block, ERROR_IN_PROGRESS);
        return;
    }
    if (size)
    {
        drv->i2c.i2cs[port]->ptr = block_open(block);
        if (drv->i2c.i2cs[port]->ptr == NULL)
        {
            fread_complete(process, HAL_HANDLE(HAL_I2C, port), block, get_last_error());
            return;
        }
    }
    drv->i2c.i2cs[port]->process = process;
    drv->i2c.i2cs[port]->block = block;
    drv->i2c.i2cs[port]->size = size;
    drv->i2c.i2cs[port]->processed = 0;
    drv->i2c.i2cs[port]->addr_processed = 0;
    drv->i2c.i2cs[port]->rx_len_processed = 0;
    drv->i2c.i2cs[port]->rx_len = 0;
    drv->i2c.i2cs[port]->io = I2C_IO_RX;

    //reset
    LPC_I2C->CONCLR = I2C_CLEAR;
    //set START
    LPC_I2C->CONSET = I2C_CONSET_I2EN | I2C_CONSET_STA;
    //all rest in isr
}

static inline void lpc_i2c_write(SHARED_I2C_DRV* drv, I2C_PORT port, HANDLE block, unsigned int size, HANDLE process)
{
    if (port >= I2C_COUNT || (size && block == INVALID_HANDLE))
    {
        fwrite_complete(process, HAL_HANDLE(HAL_I2C, port), block, ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->i2c.i2cs[port] == NULL)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_I2C, port), block, ERROR_NOT_CONFIGURED);
        return;
    }
    if (drv->i2c.i2cs[port]->io != I2C_IO_IDLE)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_I2C, port), block, ERROR_IN_PROGRESS);
        return;
    }
    if (size)
    {
        drv->i2c.i2cs[port]->ptr = block_open(block);
        if (drv->i2c.i2cs[port]->ptr == NULL)
        {
            fwrite_complete(process, HAL_HANDLE(HAL_I2C, port), block, get_last_error());
            return;
        }
    }
    drv->i2c.i2cs[port]->process = process;
    drv->i2c.i2cs[port]->block = block;
    drv->i2c.i2cs[port]->size = size;
    drv->i2c.i2cs[port]->processed = 0;
    drv->i2c.i2cs[port]->addr_processed = 0;
    drv->i2c.i2cs[port]->rx_len_processed = 0;
    drv->i2c.i2cs[port]->rx_len = 0;
    drv->i2c.i2cs[port]->io = I2C_IO_TX;

    //reset
    LPC_I2C->CONCLR = I2C_CLEAR;
    //set START
    LPC_I2C->CONSET = I2C_CONSET_I2EN | I2C_CONSET_STA;
    //all rest in isr
}

static inline void lpc_i2c_seek(SHARED_I2C_DRV* drv, I2C_PORT port, unsigned int pos)
{
    if (port >= I2C_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (drv->i2c.i2cs[port] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    drv->i2c.i2cs[port]->addr = pos;
}

void lpc_i2c_init(SHARED_I2C_DRV* drv)
{
    int i;
    for (i = 0; i < I2C_COUNT; ++i)
        drv->i2c.i2cs[i] = NULL;
}

#if (SYS_INFO)
void lpc_i2c_info(SHARED_I2C_DRV* drv)
{
    int i;
    //TODO:
    for (i = 0; i < I2C_COUNT; ++i)
    {
        if (drv->i2c.i2cs[i])
        {

        }
    }
}

#endif

bool lpc_i2c_request(SHARED_I2C_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        lpc_i2c_info(drv);
        need_post = true;
        break;
#endif
    case IPC_OPEN:
        lpc_i2c_open(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3);
        need_post = true;
        break;
    case IPC_CLOSE:
        lpc_i2c_close(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_WRITE:
        lpc_i2c_write(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //async message, no write
        break;
    case IPC_READ:
        lpc_i2c_read(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //async message, no write
        break;
    case IPC_SEEK:
        lpc_i2c_seek(drv, HAL_ITEM(ipc->param1), ipc->param2);
        need_post = true;
        break;
    default:
        break;
    }
    return need_post;
}

#if !(MONOLITH_I2C)
void lpc_i2c()
{
    SHARED_I2C_DRV drv;
    IPC ipc;
    bool need_post;
    object_set_self(SYS_OBJ_I2C);
    lpc_i2c_init(&drv);
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
        default:
            need_post = lpc_i2c_request(&drv, &ipc);
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
#endif
