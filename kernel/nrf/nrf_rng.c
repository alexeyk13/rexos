/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#include "nrf_rng.h"
#include "../../userspace/sys.h"
#include "../../userspace/io.h"
#include "../kerror.h"
#include "../ksystime.h"
#include "../kirq.h"
#include "nrf_exo_private.h"
#include "sys_config.h"

static inline uint8_t nrf_get_random_byte()
{
    return (uint8_t)(NRF_RNG->VALUE & RNG_VALUE_VALUE_Msk);
}

void nrf_rng_irq(int number, void* param)
{
    EXO* exo = (EXO*)param;
    uint8_t byte = 0;

    /* get new rng byte */
    byte = nrf_get_random_byte();
    if(byte == exo->rng.last_byte)
        return;

    /* append new data */
    io_data_append(exo->rng.io, (uint8_t*)&byte, sizeof(uint8_t));
    /* save last byte */
    exo->rng.last_byte = byte;
    /* verify total bytes */
    if(io_get_free(exo->rng.io) == 0)
    {
        /* clear interrupt bit */
        NRF_RNG->INTENCLR = RNG_INTENCLR_VALRDY_Msk;
        /* stop RNG */
        NRF_RNG->TASKS_STOP = 1;
        /* disable IRQ and return iio_complete */
        NVIC_DisableIRQ(RNG_IRQn);
        /* send data back */
        iio_complete(exo->rng.user, HAL_IO_CMD(HAL_RNG, IPC_READ), 0, exo->rng.io);
        exo->rng.io = NULL;
        exo->rng.user = INVALID_HANDLE;
    }
}


void nrf_rng_read(EXO* exo, HANDLE user, IO* io)
{
    /* io is NULL */
    if(io == NULL)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    /* already wait data */
    if(exo->rng.user != INVALID_HANDLE)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    /* save user process */
    exo->rng.user = user;
    /* save io */
    exo->rng.io = io;
    /* enable IRQ */
    NRF_RNG->INTENSET = RNG_INTENSET_VALRDY_Msk;
    /* IRQ Enable */
    NVIC_EnableIRQ(RNG_IRQn);
    /* start RNG */
    NRF_RNG->TASKS_START = 1;
    /* all rest in IRQ */
    kerror(ERROR_SYNC);
}

void nrf_rng_init(EXO* exo)
{
    exo->rng.io = NULL;
    exo->rng.user = INVALID_HANDLE;
    exo->rng.last_byte = 0;
    /* enable digital error correction */
    /* decrease speed, but increase random quality */
    NRF_RNG->CONFIG |= RNG_CONFIG_DERCEN_Msk;

    /* register irq */
    kirq_register(KERNEL_HANDLE, RNG_IRQn, nrf_rng_irq, (void*)exo);
}

void nrf_rng_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
        case IPC_READ:
            nrf_rng_read(exo, ipc->process, (IO*)ipc->param2);
            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
            break;
    }
}

