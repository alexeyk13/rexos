/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#include "nrf_wdt.h"
#include "../../userspace/sys.h"
#include "../../userspace/wdt.h"
#include "../kerror.h"
#include "sys_config.h"

#define WDT_RELOAD_KEY                  0x6E524635
#define WDT_CLOCK_FREQ_1S               (32768)

static inline void nrf_wdt_reload(uint8_t rr_num)
{
    NRF_WDT->RR[rr_num] = WDT_RELOAD_KEY;
}

void nrf_wdt_pre_init()
{
    /* power on */
    NRF_WDT->POWER = 1;
    NRF_WDT->TASKS_START = 0;
    /* init WDT timeout */
    NRF_WDT->CRV = WDT_CLOCK_FREQ_1S * WDT_TIMEOUT_S;
    /* wdt enable during sleep and halt */
    NRF_WDT->CONFIG =
        WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos |
        WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos;
    /* enable 0 RR */
    NRF_WDT->RREN |= WDT_RREN_RR0_Msk;
    /* start */
    NRF_WDT->TASKS_START = 1;
}

void nrf_wdt_init_reload_register(uint8_t rr_num, unsigned int timeout_s)
{
    NRF_WDT->RREN |= (WDT_RREN_RR0_Enabled << rr_num);
}

void nrf_wdt_init()
{
    /* Do nothing */
}

void nrf_wdt_request(IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        nrf_wdt_init_reload_register(ipc->param1, ipc->param2);
        break;
    case WDT_KICK:
        nrf_wdt_reload(ipc->param1);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
