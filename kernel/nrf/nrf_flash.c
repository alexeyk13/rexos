/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#include "nrf_flash.h"
#include "nrf_exo_private.h"
#include "nrf_config.h"
#include "../kerror.h"

static inline void flash_erase_page(uint32_t * page_address)
{
    // Turn on flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Erase page:
    NRF_NVMC->ERASEPAGE = (uint32_t)page_address;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Turn off flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}

static inline void flash_write_word(uint32_t * address, uint32_t value)
{
    // Turn on flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    *address = value;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
    // Turn off flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}

static inline void flash_io(EXO* exo, IPC* ipc)
{

}

unsigned int flash_page_size()
{
    return (NRF_FICR->CODEPAGESIZE);
}

unsigned int flash_total_size()
{
    return (NRF_FICR->CODESIZE);
}

void nrf_flash_init(EXO* exo)
{
    // Init
}

void nrf_flash_request(EXO* exo, IPC* ipc)
{
    // Flash request
    switch (HAL_ITEM(ipc->cmd))
    {
        case IPC_READ:
            flash_io(exo, ipc);
            break;
        case IPC_WRITE:
            flash_io(exo, ipc);
            break;
        case FLASH_GET_PAGE_SIZE:
            ipc->param3 = flash_page_size();
            break;
        case FLASH_GET_TOTAL_SIZE:
            ipc->param3 = flash_total_size();
            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
    }
}
