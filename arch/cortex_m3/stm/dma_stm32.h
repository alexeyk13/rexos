/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DMA_STM32_H
#define DMA_STM32_H

#include "dev.h"
#include "arch.h"

//dma flags
#define DMA_FEIF                                (1 << 0)
#define DMA_DMEIF                                (1 << 2)
#define DMA_TEIF                                (1 << 3)
#define DMA_HTIF                                (1 << 4)
#define DMA_TCIF                                (1 << 5)

//dma stream CR

#define DMA_STREAM_CR_CHANNEL_0            (0 << 25)
#define DMA_STREAM_CR_CHANNEL_1            (1 << 25)
#define DMA_STREAM_CR_CHANNEL_2            (2 << 25)
#define DMA_STREAM_CR_CHANNEL_3            (3 << 25)
#define DMA_STREAM_CR_CHANNEL_4            (4 << 25)
#define DMA_STREAM_CR_CHANNEL_5            (5 << 25)
#define DMA_STREAM_CR_CHANNEL_6            (6 << 25)
#define DMA_STREAM_CR_CHANNEL_7            (7 << 25)

#define DMA_STREAM_CR_MBURST_SINGLE        (0 << 23)
#define DMA_STREAM_CR_MBURST_INCR4        (1 << 23)
#define DMA_STREAM_CR_MBURST_INCR8        (2 << 23)
#define DMA_STREAM_CR_MBURST_INCR16        (3 << 23)

#define DMA_STREAM_CR_PBURST_SINGLE        (0 << 21)
#define DMA_STREAM_CR_PBURST_INCR4        (1 << 21)
#define DMA_STREAM_CR_PBURST_INCR8        (2 << 21)
#define DMA_STREAM_CR_PBURST_INCR16        (3 << 21)

#define DMA_STREAM_CR_CT_MEMORY_0        (0 << 19)
#define DMA_STREAM_CR_CT_MEMORY_1        (1 << 19)

#define DMA_STREAM_CR_PL_LOW                (0 << 16)
#define DMA_STREAM_CR_PL_MEDIUM            (1 << 16)
#define DMA_STREAM_CR_PL_HIGH                (2 << 16)
#define DMA_STREAM_CR_PL_VERY_HIGH        (3 << 16)

#define DMA_STREAM_CR_MSIZE_BYTE            (0 << 13)
#define DMA_STREAM_CR_MSIZE_HALF_WORD    (1 << 13)
#define DMA_STREAM_CR_MSIZE_WORD            (2 << 13)

#define DMA_STREAM_CR_PSIZE_BYTE            (0 << 11)
#define DMA_STREAM_CR_PSIZE_HALF_WORD    (1 << 11)
#define DMA_STREAM_CR_PSIZE_WORD            (2 << 11)

#define DMA_STREAM_CR_DIR_P2M                (0 << 6)
#define DMA_STREAM_CR_DIR_M2P                (1 << 6)
#define DMA_STREAM_CR_DIR_M2M                (2 << 6)

//dma stream fcr

#define DMA_STREAM_FCR_FS_0_25            (0 << 3)
#define DMA_STREAM_FCR_FS_25_50            (1 << 3)
#define DMA_STREAM_FCR_FS_50_75            (2 << 3)
#define DMA_STREAM_FCR_FS_75_FULL        (3 << 3)
#define DMA_STREAM_FCR_FS_EMPTY            (4 << 3)
#define DMA_STREAM_FCR_FS_FULL            (5 << 3)

#define DMA_STREAM_FCR_FTH_25                (0 << 0)
#define DMA_STREAM_FCR_FTH_50                (1 << 0)
#define DMA_STREAM_FCR_FTH_75                (2 << 0)
#define DMA_STREAM_FCR_FTH_FULL            (3 << 0)

typedef struct {
    DMA_TypeDef DMA;
    DMA_Stream_TypeDef STREAM[8];
}DMA_FULL_TypeDef, *DMA_FULL_TypeDef_P;

#define DMA1_FULL                                ((DMA_FULL_TypeDef *) DMA1_BASE)
#define DMA2_FULL                                ((DMA_FULL_TypeDef *) DMA2_BASE)

extern const DMA_FULL_TypeDef_P DMA[];

void dma_enable(DMA_CLASS dma);
void dma_disable(DMA_CLASS dma);

unsigned long dma_get_flags(DMA_CLASS dma, DMA_STREAM_CLASS stream);
void dma_clear_flags(DMA_CLASS dma, DMA_STREAM_CLASS stream, unsigned long flags);

#endif // DMA_STM32_H
