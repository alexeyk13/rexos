/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_analog.h"
#include "stm32_power.h"
#include "stm32_config.h"
#include "../../userspace/direct.h"
#include "../../userspace/block.h"
#include "../sys.h"

#define ADC1_CLOCK_MAX              14000000
#define ADC1_TSTAB                  11
#define ADC_SQR_LEN(len)            (((len - 1ul) & 0xful) << 20ul)
#define ADC2uV(raw)					((raw * ADC_VREF * 100 / 0xfff) * 10ul)

#define DAC_TWAKEUP                 10
const PIN DAC_OUT[] =               {A4, A5};

void stm32_analog();

const REX __STM32_ANALOG = {
    //name
    "STM32 analog",
    //size
    512,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_analog
};

typedef struct {
    bool active;
    DAC_TRIGGER trigger;
    union {
        PIN pin;
        TIMER_NUM timer;
    } u;
    HANDLE block, process;
    void* ptr;
    unsigned int size;
} DAC_STRUCT;

typedef struct {
    DAC_STRUCT dac[2];
    int active_channels;
} ANALOG;

static inline void stm32_adc_open()
{
    unsigned int apb2, psc;
    HANDLE core = sys_get_object(SYS_OBJECT_CORE);
    if (core == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    apb2 = get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_APB2, 0, 0);
    for(psc = 2; psc < 8 && apb2 / psc > ADC1_CLOCK_MAX; psc += 2) {}
    RCC->CFGR &= ~(3 << 14);
    RCC->CFGR |= (psc / 2 - 1) << 14;
    //enable clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    //turn ADC1 on
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_TSVREFE;
    sleep_us(ADC1_TSTAB);

    //start self-calibration
    ADC1->CR2 |= ADC_CR2_CAL;
    while(ADC1->CR2 & ADC_CR2_CAL) {}
}

void stm32_adc_set_sequence_channel(int sq, int chan)
{
    if (sq >= 13)
    {
        ADC1->SQR1 &= ~(0x1ful << ((sq - 13ul) * 5ul));
        ADC1->SQR1 |= chan << ((sq - 13ul) * 5ul);
    }
    else if (sq >= 7)
    {
        ADC1->SQR2 &= ~(0x1ful << ((sq - 7ul) * 5ul));
        ADC1->SQR2 |= chan << ((sq - 7ul) * 5ul);
    }
    else //1..6
    {
        ADC1->SQR3 &= ~(0x1ful << ((sq - 1ul) * 5ul));
        ADC1->SQR3 |= chan << ((sq - 1ul) * 5ul);
    }
}

void stm32_adc_set_sample_rate(int chan, int sample_rate)
{
    if (chan >= 10)
    {
        ADC1->SMPR1 &= ~(0x7ul << ((chan - 10ul) * 3ul));
        ADC1->SMPR1 |= sample_rate << ((chan - 10ul) * 3ul);
    }
    else
    {
        ADC1->SMPR2 &= ~(0x7ul << (chan * 3ul));
        ADC1->SMPR2 |= sample_rate << (chan * 3ul);
    }
}

int stm32_adc_get_single_sample(int chan, int sample_rate)
{
    stm32_adc_set_sample_rate(chan, sample_rate);

    //sequence len = 1
    ADC1->SQR1 = ADC_SQR_LEN(1);
    stm32_adc_set_sequence_channel(1, chan);

    //start conversion cycle
    ADC1->SR = 0;
    ADC1->CR2 |= ADC_CR2_ADON;
    while ((ADC1->SR & ADC_SR_EOC) == 0) {}

    return ADC1->DR;
}

void stm32_dac_open_channel(ANALOG* analog, HANDLE core, int channel, STM32_DAC_ENABLE* dac_enable)
{
    uint32_t reg;
    //enable PIN analog mode
    ack(core, STM32_GPIO_ENABLE_PIN_SYSTEM, DAC_OUT[channel], GPIO_MODE_INPUT_ANALOG, false);
    //EN
    DAC->CR &= ~(0xffff << (16 * channel));
    reg = (1 << 0);
#if (DAC_BOFF)
    reg |= (1 << 1);
#endif
    DAC->CR |= reg << (16 * channel);
}

void stm32_dac_close_channel(ANALOG* analog, HANDLE core, int channel)
{
    if (analog->dac[channel].active)
    {
        analog->dac[channel].active = false;
        //DIS
        DAC->CR &= ~(0xffff << (16 * channel));
        //disable PIN
        ack(core, STM32_GPIO_DISABLE_PIN, DAC_OUT[channel], 0, 0);
    }
}

void stm32_dac_open(ANALOG* analog, STM32_DAC_ENABLE* dac_enable)
{
    HANDLE core = sys_get_object(SYS_OBJECT_CORE);
    if (core == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    //turn clock on
    if (analog->active_channels++ == 0)
        RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    if (dac_enable->dac == STM32_DAC1 || dac_enable->dac == STM32_DAC_DUAL)
        stm32_dac_open_channel(analog, core, 0, dac_enable);
    if (dac_enable->dac == STM32_DAC2 || dac_enable->dac == STM32_DAC_DUAL)
        stm32_dac_open_channel(analog, core, 1, dac_enable);

    sleep_us(DAC_TWAKEUP);
}

void stm32_dac_close(ANALOG* analog, STM32_DAC dac)
{
    HANDLE core = sys_get_object(SYS_OBJECT_CORE);
    if (core == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    //TODO: flush
    //disable channel
    if (dac == STM32_DAC1 || dac == STM32_DAC_DUAL)
        stm32_dac_close_channel(analog, core, 0);
    if (dac == STM32_DAC2 || dac == STM32_DAC_DUAL)
        stm32_dac_close_channel(analog, core, 1);
    //turn clock off
    if (--analog->active_channels == 0)
        RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void stm32_dac_write(ANALOG* analog, IPC* ipc)
{
    if (ipc->param1 > STM32_DAC_DUAL)
    {
        ipc_post_error(ipc->process, ERROR_INVALID_PARAMS);
        return;
    }
    DAC_STRUCT* ds = &analog->dac[ipc->param1 == STM32_DAC2 ? 1 : 0];
    if (ds->active)
    {
        ipc_post_error(ipc->process, ERROR_IN_PROGRESS);
        return;
    }
    ds->block = ipc->param2;
    if ((ds->ptr = block_open(ds->block)) == NULL)
    {
        ipc_post_error(ipc->process, get_last_error());
        return;
    }
    ds->process = ipc->process;
    ds->size = ipc->param3;
    ds->active = true;

    //just for test
    volatile int i, j;
    __disable_irq();
    for (i = 0; i < ds->size; ++i)
    {
        DAC->DHR12R1 = ((uint32_t*)(ds->ptr))[i];
        for (j = 0; j < 500; ++j) {}
    }
    __enable_irq();

    ipc->process = ds->process;
    ipc->cmd = IPC_WRITE_COMPLETE;
    ipc->param1 = STM32_DAC1;
    ipc->param2 = ds->block;
    ipc->param3 = ds->size;
    block_send_ipc(ds->block, ds->process, ipc);
    ds->block = INVALID_HANDLE;

    ds->active = false;
}

void stm32_analog()
{
    IPC ipc;
    ANALOG analog;
    STM32_DAC_ENABLE de;
    analog.dac[0].active = analog.dac[1].active = false;
    analog.active_channels = 0;
#if (SYS_INFO)
    open_stdout();
#endif

    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case IPC_CALL_ERROR:
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
//            stm32_adc_info(uarts);
            ipc_post(&ipc);
            break;
#endif
        case STM32_ADC_SINGLE_CHANNEL:
            ipc.param1 = stm32_adc_get_single_sample(ipc.param1, ipc.param2);
            ipc_post(&ipc);
            break;
        case STM32_ADC_TEMP:
            ipc.param1 = (V25_MV * 1000 - ADC2uV(stm32_adc_get_single_sample(STM32_TEMP_SENSOR, ADC_SMPR_239_5))) * 10 / AVG_SLOPE + 25l * 10l;
            ipc_post(&ipc);
            break;
        case IPC_OPEN:
            if (ipc.param1 == STM32_ADC)
                stm32_adc_open();
            else if (ipc.param1 < STM32_ADC)
            {
                if (direct_read(ipc.process, (void*)&de, sizeof(STM32_DAC_ENABLE)))
                    stm32_dac_open(&analog, &de);
            }
            else
                error(ERROR_INVALID_PARAMS);
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
            if (ipc.param1 < STM32_ADC)
                stm32_dac_close(&analog, ipc.param1);
            else
                error(ERROR_INVALID_PARAMS);
            ipc_post_or_error(&ipc);
            break;
        case IPC_WRITE:
            stm32_dac_write(&analog, &ipc);
            //generally posted with block, no return IPC
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
