/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/


#include "nrf_adc.h"
#include "../../userspace/sys.h"
#include "../../userspace/io.h"
#include "../../userspace/adc.h"
#include "../kerror.h"
#include "nrf_exo_private.h"
#include "sys_config.h"

static int nrf_adc_get(EXO* exo, NRF_ADC_AIN ain, unsigned int samplerate)
{
    if (!exo->adc.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return 0;
    }
    /* samplerate do not need here */

    /* flush current AIN pin */
    NRF_ADC->CONFIG &= ~(ADC_CONFIG_PSEL_Msk);
    /* set AIN */
    NRF_ADC->CONFIG |= ((1 << ain) << ADC_CONFIG_PSEL_Pos);
    /* launch measurment */
    NRF_ADC->TASKS_START = 1;
    /* wait measurement */
    while(NRF_ADC->EVENTS_END == 0);
    /* return result */
    return NRF_ADC->RESULT;
}

void nrf_adc_init(EXO* exo)
{
    exo->adc.active = false;
}

static inline void nrf_adc_open_device(EXO* exo)
{
    if (exo->adc.active)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    /* Config ADC */
    NRF_ADC->CONFIG = (NRF_ADC_REFERENCE << ADC_CONFIG_REFSEL_Pos)
                        | (NRF_ADC_INPUT << ADC_CONFIG_INPSEL_Pos)
                        | (NRF_ADC_RESOLUTION << ADC_CONFIG_RES_Pos);

    /* Enable ADC */
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

    exo->adc.active = true;
}

static inline void nrf_adc_close_device(EXO* exo)
{
    if (!exo->adc.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    /* turn ADC off */
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Disabled;
    exo->adc.active = false;
}

void nrf_adc_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case ADC_GET:
        ipc->param2 = nrf_adc_get(exo, ipc->param1, ipc->param2);
        break;
    case IPC_OPEN:
        if (ipc->param1 == ADC_HANDLE_DEVICE)
            nrf_adc_open_device(exo);
        break;
    case IPC_CLOSE:
        if (ipc->param1 == ADC_HANDLE_DEVICE)
            nrf_adc_close_device(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
