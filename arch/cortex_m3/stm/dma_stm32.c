/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "dma_stm32.h"
#include "error.h"

const DMA_FULL_TypeDef_P DMA[] =                    {DMA1_FULL, DMA2_FULL};
const uint32_t RCC_DMA[] =                            {RCC_AHB1ENR_DMA1EN, RCC_AHB1ENR_DMA2EN};
const uint32_t DMA_SHIFT_MASK[] =                {0x0, 0x6, 0x10, 0x16};

unsigned long _dma_enabled[DMA_COUNT]   =        {0};

void dma_enable(DMA_CLASS dma)
{
    if (dma < DMA_COUNT)
    {
        if (_dma_enabled[dma]++ == 0)
            RCC->AHB1ENR |= RCC_DMA[dma];
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void dma_disable(DMA_CLASS dma)
{
    if (dma < DMA_COUNT)
    {
        if (--_dma_enabled[dma] == 0)
            RCC->AHB1ENR &= ~RCC_DMA[dma];
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

unsigned long dma_get_flags(DMA_CLASS dma, DMA_STREAM_CLASS stream)
{
    unsigned long res = 0;
    if (stream < DMA_STREAM_4)
        res = DMA[dma]->DMA.LISR;
    else
        res = DMA[dma]->DMA.HISR;
    res = (res >> DMA_SHIFT_MASK[(stream & 3)]) & 0x3f;
    return res;
}

void dma_clear_flags(DMA_CLASS dma, DMA_STREAM_CLASS stream, unsigned long flags)
{
    unsigned long res = (flags & 0x3f) << DMA_SHIFT_MASK[(stream & 3)];
    if (stream < DMA_STREAM_4)
        DMA[dma]->DMA.LIFCR = res;
    else
        DMA[dma]->DMA.HIFCR = res;
}
