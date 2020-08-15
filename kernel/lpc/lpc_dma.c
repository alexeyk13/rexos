#include "lpc_dma.h"

#include "sys_config.h"
#include <string.h>

void dma_open()
{
    LPC_GPDMA->CONFIG = 1;
}

void dma_set_channel(uint32_t ch, DMA_CH_DESC* desc)
{
    uint32_t ctrl, mux_offs;
    LPC_GPDMA_CH_Type* chptr = &((*LPC_GPDMA_CH)[ch]);
    chptr->LLI = 0;

    ctrl = (desc->burst << GPDMA_C0CONTROL_SBSIZE_Pos) | (desc->burst << GPDMA_C0CONTROL_DBSIZE_Pos)
         | (desc->width << GPDMA_C0CONTROL_SWIDTH_Pos) | (desc->width << GPDMA_C0CONTROL_DWIDTH_Pos);

    chptr->CONFIG = (desc->perif_ch << GPDMA_C0CONFIG_SRCPERIPHERAL_Pos) | (desc->perif_ch << GPDMA_C0CONFIG_DESTPERIPHERAL_Pos) | (desc->dir << GPDMA_C0CONFIG_FLOWCNTRL_Pos);
    mux_offs = desc->perif_ch << 1;
    switch (desc->dir)
    {
    case DMA_MEM_TO_MEM:
        break;
    case DMA_MEM_TO_PERIF:
        LPC_CREG->DMAMUX = (LPC_CREG->DMAMUX & ~(0x03 << mux_offs)) | ((desc->perif_mux &0x03) << mux_offs);
        ctrl |= GPDMA_C0CONTROL_D_Msk | GPDMA_C0CONTROL_SI_Msk;
        chptr->DESTADDR = desc->perif_addr;
        break;
    case DMA_PERIF_TO_MEM:
        LPC_CREG->DMAMUX = (LPC_CREG->DMAMUX & (0x03 << mux_offs)) | ((desc->perif_mux &0x03) << mux_offs);
        ctrl |= GPDMA_C0CONTROL_S_Msk | GPDMA_C0CONTROL_DI_Msk;
        chptr->SRCADDR = desc->perif_addr;
        break;
    case DMA_PERIF_TO_PREIF:
        break;
    }
    chptr->CONTROL = ctrl;
}

void dma_enable_perif_channel(uint32_t ch, void* mem, uint32_t size)
{
    LPC_GPDMA_CH_Type* chptr = &((*LPC_GPDMA_CH)[ch]);
    uint32_t cfg;

    size &= GPDMA_C0CONTROL_TRANSFERSIZE_Msk;
    cfg = chptr->CONFIG;
    chptr->CONFIG = 0;
    chptr->CONTROL = (chptr->CONTROL & ~GPDMA_C0CONTROL_TRANSFERSIZE_Msk) | size;
    if((cfg & GPDMA_C0CONFIG_FLOWCNTRL_Msk) == (DMA_PERIF_TO_MEM << GPDMA_C0CONFIG_FLOWCNTRL_Pos))
        chptr->DESTADDR = (uint32_t)mem;
    else
        chptr->SRCADDR = (uint32_t)mem;
    chptr->CONFIG = cfg | GPDMA_C0CONFIG_E_Msk;
}

void dma_enable_mem_channel(uint32_t ch, void* src, void* dst, uint32_t size)
{

}
