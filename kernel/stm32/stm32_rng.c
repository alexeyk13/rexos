/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "stm32_rng.h"
#include "stm32_exo_private.h"
#include "../kerror.h"
#include "../kirq.h"

void stm32_rng_isr(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    int i;
    uint32_t sr = RNG->SR;
    uint32_t* ptr;
    if(sr & RNG_SR_CEIS)
    {
        for(i = 0; i < 12; i++)
            RNG->DR;
        RNG->SR = 0;
        return;
    }
    if(exo->rng.io == NULL)
    {
        RNG->CR = 0;
        return;
    }

    ptr = io_data(exo->rng.io) + exo->rng.readed;
    while(RNG->SR & RNG_SR_DRDY)
    {
        *ptr++ = RNG->DR;
        exo->rng.readed += 4;
        if(exo->rng.readed >= exo->rng.io->data_size)
        {
            RNG->SR = 0;
            RNG->CR = 0;
            iio_complete(exo->rng.process, HAL_IO_CMD(HAL_RNG, IPC_READ), 0, exo->rng.io);
            exo->rng.io = NULL;
            return;
        }
    }
}


static void stm32_rng_read(EXO* exo, IPC* ipc)
{
    if(exo->rng.io != NULL)
    {
        kerror(ERROR_BUSY);
        return;
    }
    exo->rng.process = ipc->process;
    exo->rng.io = (IO*)ipc->param2;
    exo->rng.io->data_size = ipc->param3;
    exo->rng.readed = 0;

    RNG->SR = 0;
    RNG->CR = RNG_CR_RNGEN | RNG_CR_IE | RNG_CR_CED;

    kerror(ERROR_SYNC);
}


void stm32_rng_init(EXO* exo)
{
    RCC->D2CCIP2R = (RCC->D2CCIP2R & ~RCC_D2CCIP2R_RNGSEL_Msk) | RNG_CLOCK_SRC;
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    RNG->CR = 0;
    exo->rng.io = NULL;

    kirq_register(KERNEL_HANDLE, RNG_IRQn, stm32_rng_isr, exo);
    NVIC_EnableIRQ(RNG_IRQn);
    NVIC_SetPriority(RNG_IRQn, 13);
}

void stm32_rng_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        stm32_rng_read(exo, ipc);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
