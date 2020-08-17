#ifndef LPC_DMA_H
#define LPC_DMA_H

#include "../../userspace/lpc/lpc_driver.h"
#include <string.h>
#include <stdint.h>

void dma_open();
void dma_set_channel(uint32_t ch, DMA_CH_DESC* desc);
void dma_enable_perif_channel(uint32_t ch, void* mem, uint32_t size);
void dma_enable_mem_channel(uint32_t ch, void* src, void* dst, uint32_t size);

static inline void dma_disable_channel(uint32_t ch)
{
    (*LPC_GPDMA_CH)[ch].CONFIG &=  ~GPDMA_C0CONFIG_E_Msk;
}
static inline uint32_t  dma_get_transfer_size(uint32_t ch)
{
    return (*LPC_GPDMA_CH)[ch].CONTROL & GPDMA_C0CONTROL_TRANSFERSIZE_Msk;
}

#endif // LPC_DMA_H
