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
#if defined(NRF51)
#define HFCLK_RC_FREQ                       16000000
#elif defined (NRF52)
// FIXME: clock configuration and clock cpu value returning
#define HFCLK_RC_FREQ                       16000000
#endif // NRF52


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

#if (NRF_DECODE_RESET)
static inline void decode_reset_reason(EXO* exo)
{
    exo->power.reset_reason = NRF_POWER->RESETREAS;
}
#endif // NRF_DECODE_RESET

void nrf_power_init(EXO* exo)
{
    hfclk_start();

#if (LFCLK)
    lfclk_start();
#endif // LFCLK

#if (NRF_DECODE_RESET)
    decode_reset_reason(exo);
#endif //STM32_DECODE_RESET

#if defined(NRF51)
    /* periph power */
#if !(NRF_ADC_DRIVER)
    NRF_ADC->POWER = 0;
#endif // NRF_ADC_DRIVER
#if !(NRF_WDT_DRIVER)
    NRF_WDT->POWER = 0;
#endif // NRF_WDT_DRIVER
#if !(NRF_SPI_DRIVER)
    NRF_SPI0->POWER = 0;
    NRF_SPI1->POWER = 0;
#endif // NRF_SPI_DRIVER
#if !(NRF_RTC_DRIVER)
    NRF_RTC0->POWER = 0;
    NRF_RTC1->POWER = 0;
#endif // NRF_RTC_DRIVER
#endif // NRF51

    /* SRAM power config */
#if (NRF_SRAM_POWER_CONFIG)
#if defined(NRF51)
#if (NRF_RAM1_ENABLE)
    NRF_POWER->RAMON |= POWER_RAMON_ONRAM1_Msk;
#else
    NRF_POWER->RAMON &= ~(POWER_RAMON_ONRAM1_Msk);
#endif // NRF_RAM1_ENABLE
#endif // NRF51

#if defined(NRF52)
#if !(NRF_RAM7_ENABLE)
#endif // NRF_RAM7_ENABLE
#if !(NRF_RAM6_ENABLE)
#endif // NRF_RAM6_ENABLE
#if !(NRF_RAM5_ENABLE)
#endif // NRF_RAM5_ENABLE
#if !(NRF_RAM4_ENABLE)
#endif // NRF_RAM4_ENABLE
#if !(NRF_RAM3_ENABLE)
#endif // NRF_RAM3_ENABLE
#if !(NRF_RAM2_ENABLE)
#endif // NRF_RAM2_ENABLE
#if !(NRF_RAM1_ENABLE)
#endif // NRF_RAM1_ENABLE
#endif // NRF52
#endif // NRF_SRAM_POWER_CONFIG

#if (NRF_SRAM_RETENTION_ENABLE)
    /* keep retention during system OFF */
    NRF_POWER->RAMON |= POWER_RAMON_OFFRAM0_Msk;
#if (!(NRF_SRAM_POWER_CONFIG) || (NRF_RAM1_ENABLE))
    NRF_POWER->RAMON |= POWER_RAMON_OFFRAM1_Msk;
#endif // NRF_RAM1_ENABLE
#else
    /* do not keep retention during system OFF */
    NRF_POWER->RAMON &= ~(POWER_RAMON_OFFRAM0_Msk);
    NRF_POWER->RAMON &= ~(POWER_RAMON_OFFRAM1_Msk);
#endif // NRF_SRAM_RETENTION_ENABLE
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

#if (POWER_MANAGEMENT)
static inline void nrf_power_set_mode(EXO* exo, POWER_MODE mode)
{
    switch(mode)
    {
        case POWER_MODE_STOP:
            /* Enter system OFF. */
            /* Typycal consumption 60.8 uA without SRAM retention */
            /* After wakeup the chip will be reset */
            /* Code execution will run from the scratch */
            NRF_POWER->SYSTEMOFF = 1;
            break;
        case POWER_MODE_STANDY:
            /* Enter Low Power mode */
            /* Typycal consumption 60.8 uA without SRAM retention */
            // Enter System ON sleep mode
            __WFE();
            __SEV();
            __WFE();
            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
    }
}
#endif // POWER_MANAGEMENT
void nrf_power_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case POWER_GET_CLOCK:
        ipc->param2 = get_core_clock_internal(ipc->param1);
        break;
#if (NRF_DECODE_RESET)
    case NRF_POWER_GET_RESET_REASON:
        ipc->param2 = exo->power.reset_reason;
        break;
#endif //STM32_DECODE_RESET

#if (POWER_MANAGEMENT)
    case POWER_SET_MODE:
        //no return
        nrf_power_set_mode(exo, ipc->param1);
        break;
#endif //POWER_MANAGEMENT
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}


