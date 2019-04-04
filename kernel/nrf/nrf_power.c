/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_power.h"
#include "nrf_exo_private.h"
#include "nrf_config.h"
#include "../kerror.h"

/* Internal RC generator frequency */
#define HFCLK_RC_FREQ                       16000000

static inline void hfclk_start()
{
    /* HSE already run */
    if (NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Xtal)
        return;

    /* interupt settings */
    NRF_CLOCK->INTENSET = CLOCK_INTENSET_HFCLKSTARTED_Msk; // no need here

    /* Enable wake-up on event */
    SCB->SCR |= SCB_SCR_SEVONPEND_Msk;

    /* flush event status */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    /* start HFCLK */
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    /* wait for the external oscillator to start up */
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    /* Clear the event and the pending interrupt */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

//    NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
//    NRF_CLOCK->INTENSET = 0;
}

static inline void hfclk_stop()
{
    /* stop HFCLK */
    NRF_CLOCK->TASKS_HFCLKSTOP = 1;
}

static inline void lfclk_start()
{
    if(NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Running)
        return;

    /* set source */
    NRF_CLOCK->LFCLKSRC = (LFCLK_SOURCE << CLOCK_LFCLKSRC_SRC_Pos);

    /* interupt settings */
//    NRF_CLOCK->INTENSET = CLOCK_INTENSET_LFCLKSTARTED_Msk;

    /* enable wake-up on event */
    SCB->SCR |= SCB_SCR_SEVONPEND_Msk;

    /* flush event status */
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    /* start LFCLK */
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    /* Wait for the external oscillator to start up. */
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);

    /* Clear the event and the pending interrupt */
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

//    NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
//    NRF_CLOCK->INTENSET = 0;
}

static inline void lfclk_stop()
{
    /* stop LFCLK */
    NRF_CLOCK->TASKS_LFCLKSTOP = 1;
}

void nrf_power_init(EXO* exo)
{
    hfclk_start();

#if (LFCLK)
    lfclk_start();
#endif // LFCLK
}

int get_core_clock_internal()
{
    /* External 16MHz/32MHz crystal oscillator */
    if(NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Xtal)
    {
        /* return the value of HFCLK */
        // TODO: verify NRF_CLOCK->XTALFREQ
#if (HFCLK)
        return HFCLK;
#else
        return HFCLK_RC_FREQ;
#endif // HFCLK
    }

    if(NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_RC)
        return HFCLK_RC_FREQ;

    /* dummy return - always on RC */
    return HFCLK_RC_FREQ;
}

void nrf_power_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case POWER_GET_CLOCK:
        ipc->param2 = get_core_clock_internal(ipc->param1);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}


