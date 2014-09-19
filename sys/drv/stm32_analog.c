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
#include "../../userspace/irq.h"
#include "../sys.h"

#define ADC1_CLOCK_MAX                                  14000000
#define ADC1_TSTAB                                      11
#define ADC_SQR_LEN(len)                                (((len - 1ul) & 0xful) << 20ul)
#define ADC2uV(raw)                                     ((raw * ADC_VREF * 100 / 0xfff) * 10ul)

#define DAC_TWAKEUP                                     10
const PIN DAC_OUT[] =                                   {A4, A5};
#if (DAC_DMA)
typedef DMA_Channel_TypeDef* DMA_Channel_TypeDef_P;
static const DMA_Channel_TypeDef_P DAC_DMA_REGS[2] =    {DMA2_Channel3, DMA2_Channel4};
const unsigned int DAC_DMA_VECTORS[2] =                 {58, 59};
#endif

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
    PIN pin;
    TIMER_NUM timer;
    HANDLE block, process;
    void* ptr;
    unsigned int size;
#if !(DAC_DMA)
    unsigned int* reg;
    unsigned int processed;
#endif
} DAC_STRUCT;

typedef struct {
    DAC_STRUCT dac[2];
    HANDLE core;
    int active_channels;
} ANALOG;

static inline void stm32_adc_open(ANALOG* analog)
{
    unsigned int apb2, psc;
    apb2 = get(analog->core, STM32_POWER_GET_CLOCK, STM32_CLOCK_APB2, 0, 0);
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

#if (DAC_DMA)
void stm32_dac_dma_isr(int vector, void* param)
{
    IPC ipc;
    DAC_STRUCT* dac = (DAC_STRUCT*)param;
    //TODO:
    DMA2->IFCR |= 0xf << (8 + 4 * 0);
    DAC_DMA_REGS[0]->CCR &= ~(1 << 0);

    TIMER_REGS[dac->timer]->CR1 &= ~TIM_CR1_CEN;
    dac->active = false;

    ipc.process = dac->process;
    ipc.cmd = IPC_WRITE_COMPLETE;
    ipc.param1 = STM32_DAC1;
    ipc.param2 = dac->block;
    ipc.param3 = dac->size;
    block_isend_ipc(dac->block, dac->process, &ipc);
    dac->block = INVALID_HANDLE;
}
#else
void stm32_dac_timer_isr(int vector, void* param)
{
    IPC ipc;
    DAC_STRUCT* dac = (DAC_STRUCT*)param;
    *(dac->reg) = *((unsigned int*)(dac->ptr));
    dac->ptr += sizeof(unsigned int);
    dac->processed += sizeof(unsigned int);

    if (dac->processed >= dac->size)
    {
        TIMER_REGS[dac->timer]->CR1 &= ~TIM_CR1_CEN;
        dac->active = false;

        ipc.process = dac->process;
        ipc.cmd = IPC_WRITE_COMPLETE;
        ipc.param1 = STM32_DAC1;
        ipc.param2 = dac->block;
        ipc.param3 = dac->size;
        block_isend_ipc(dac->block, dac->process, &ipc);
        dac->block = INVALID_HANDLE;

    }
    TIMER_REGS[dac->timer]->SR &= ~TIM_SR_UIF;
}
#endif

static inline void stm32_dac_open_timer(ANALOG* analog, int channel, STM32_DAC_ENABLE* de, uint32_t* reg)
{
    unsigned int psc, value, clock;
    analog->dac[channel].timer = de->timer;
#if (DAC_DMA)
    ack(analog->core, STM32_TIMER_ENABLE, de->timer, 0, 0);

    //trigger on update
    TIMER_REGS[de->timer]->CR2 = 2 << 4;
    TIMER_REGS[de->timer]->DIER |= TIM_DIER_UDE;
    switch (de->timer)
    {
    case TIM_3:
    case TIM_8:
        *reg |= 1 << 3;
        break;
    case TIM_7:
        *reg |= 2 << 3;
        break;
    case TIM_5:
        *reg |= 3 << 3;
        break;
    case TIM_2:
        *reg |= 4 << 3;
        break;
    case TIM_4:
        *reg |= 5 << 3;
        break;
    case TIM_6:
    default:
        break;
    }
#else
    irq_register(TIMER_VECTORS[de->timer], stm32_dac_timer_isr, (void*)(&(analog->dac[channel])));
    ack(analog->core, STM32_TIMER_ENABLE, de->timer, TIMER_FLAG_ENABLE_IRQ, 10);
#endif
    //setup psc
    clock = get(analog->core, STM32_TIMER_GET_CLOCK, de->timer, 0, 0);
    //period in clock units, rounded
    value = (clock * 10) / de->frequency;
    if (value % 10 >= 5)
        value += 10;
    value /= 10;
    psc = value / 0xffff;
    if (value % 0xffff)
        ++psc;

    TIMER_REGS[de->timer]->PSC = psc - 1;
    TIMER_REGS[de->timer]->ARR = (value / psc) - 1;
}

static inline void stm32_dac_write_timer(ANALOG* analog, int channel)
{
    TIMER_NUM timer = analog->dac[channel].timer;
    TIMER_REGS[timer]->CNT = 0;
    TIMER_REGS[timer]->EGR = TIM_EGR_UG;
    TIMER_REGS[timer]->CR1 |= TIM_CR1_CEN;
}

void stm32_dac_open_channel(ANALOG* analog, int channel, STM32_DAC_ENABLE* de)
{
    uint32_t reg;
    //enable PIN analog mode
    ack(analog->core, STM32_GPIO_ENABLE_PIN_SYSTEM, DAC_OUT[channel], GPIO_MODE_INPUT_ANALOG, false);
    DAC->CR &= ~(0xffff << (16 * channel));
    //EN
    reg = (1 << 0);
#if (DAC_BOFF)
    reg |= (1 << 1);
#endif
#if (DAC_DMA)
    DMA2->IFCR |= 0xf << (8 + 4 * channel);
    //DMAEN, TEN
    reg |= (1 << 12) | (1 << 2);

    //register DMA isr
    irq_register(DAC_DMA_VECTORS[channel], stm32_dac_dma_isr, (void*)(&(analog->dac[channel])));
    NVIC_EnableIRQ(DAC_DMA_VECTORS[channel]);
    NVIC_SetPriority(DAC_DMA_VECTORS[channel], 10);
#endif

    analog->dac[channel].trigger = de->trigger;
    switch (analog->dac[channel].trigger)
    {
    case DAC_TRIGGER_TIMER:
        stm32_dac_open_timer(analog, channel, de, &reg);
        break;
    default:
        break;
    }

    DAC->CR |= reg << (16 * channel);
}

void stm32_dac_close_channel(ANALOG* analog, int channel)
{
    if (analog->dac[channel].active)
    {
        analog->dac[channel].active = false;
        //DIS
        DAC->CR &= ~(0xffff << (16 * channel));
        //disable PIN
        ack(analog->core, STM32_GPIO_DISABLE_PIN, DAC_OUT[channel], 0, 0);
    }
}

void stm32_dac_open(ANALOG* analog, STM32_DAC dac, STM32_DAC_ENABLE* de)
{
    //turn clock on
    if (analog->active_channels++ == 0)
        RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    if (dac == STM32_DAC1 || dac == STM32_DAC_DUAL)
        stm32_dac_open_channel(analog, 0, de);
    if (dac == STM32_DAC2 || dac == STM32_DAC_DUAL)
        stm32_dac_open_channel(analog, 1, de);
#if (DAC_DMA)
    ack(analog->core, STM32_POWER_DMA_ON, DMA_2, 0, 0);
    switch (dac)
    {
    case STM32_DAC1:
        DAC_DMA_REGS[0]->CPAR = (unsigned int)&(DAC->DHR12R1);
        break;
    case STM32_DAC2:
        DAC_DMA_REGS[1]->CPAR = (unsigned int)&(DAC->DHR12R2);
        break;
    case STM32_DAC_DUAL:
        DAC_DMA_REGS[0]->CPAR = (unsigned int)&(DAC->DHR12RD);
        break;
    default:
        break;
    }
#else
    switch (dac)
    {
    case STM32_DAC1:
        analog->dac[0].reg = (unsigned int*)&(DAC->DHR12R1);
        break;
    case STM32_DAC2:
        analog->dac[1].reg = (unsigned int*)&(DAC->DHR12R2);
        break;
    case STM32_DAC_DUAL:
        analog->dac[0].reg = (unsigned int*)&(DAC->DHR12RD);
        break;
    default:
        break;
    }
#endif

    sleep_us(DAC_TWAKEUP);
}

void stm32_dac_close(ANALOG* analog, STM32_DAC dac)
{
    //TODO: flush
    //disable channel
    if (dac == STM32_DAC1 || dac == STM32_DAC_DUAL)
        stm32_dac_close_channel(analog, 0);
    if (dac == STM32_DAC2 || dac == STM32_DAC_DUAL)
        stm32_dac_close_channel(analog, 1);
    //turn clock off
    if (--analog->active_channels == 0)
        RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void stm32_dac_write_block(ANALOG* analog, int channel)
{
//    IPC ipc;
    DAC_STRUCT* dac = &analog->dac[channel];

    dac->active = true;
#if (DAC_DMA)
    DAC_DMA_REGS[channel]->CMAR = (unsigned int)(dac->ptr);
    DAC_DMA_REGS[channel]->CNDTR = dac->size / sizeof(uint32_t);
    //High priority, mem: 32, pf: 32, PINC, from memory, TCIE
    DAC_DMA_REGS[channel]->CCR = (2 << 12) | (2 << 10) | (2 << 8) | (1 << 7) | (1 << 4) | (1 << 1) | (1 << 0);
#endif

}

void stm32_dac_write(ANALOG* analog, IPC* ipc)
{
    if (ipc->param1 > STM32_DAC_DUAL)
    {
        ipc_post_error(ipc->process, ERROR_INVALID_PARAMS);
        return;
    }
    int channel = ipc->param1 == STM32_DAC2 ? 1 : 0;
    DAC_STRUCT* dac = &analog->dac[channel];
    if (dac->active)
    {
        ipc_post_error(ipc->process, ERROR_IN_PROGRESS);
        return;
    }
    dac->block = ipc->param2;
    if ((dac->ptr = block_open(dac->block)) == NULL)
    {
        ipc_post_error(ipc->process, get_last_error());
        return;
    }
    dac->process = ipc->process;
    dac->size = ipc->param3;

    stm32_dac_write_block(analog, channel);

    switch (dac->trigger)
    {
    case DAC_TRIGGER_TIMER:
        stm32_dac_write_timer(analog, channel);
        break;
    default:
        break;
    }

#if !(DAC_DMA)
    dac->processed = 0;
#endif

}

void stm32_analog()
{
    IPC ipc;
    ANALOG analog;
    STM32_DAC_ENABLE de;
    analog.dac[0].active = analog.dac[1].active = false;
    analog.active_channels = 0;
    analog.core = sys_get_object(SYS_OBJECT_CORE);
    if (analog.core == INVALID_HANDLE)
        process_exit();
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
//            stm32_analog_info();
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
                stm32_adc_open(&analog);
            else if (ipc.param1 < STM32_ADC)
            {
                if (direct_read(ipc.process, (void*)&de, sizeof(STM32_DAC_ENABLE)))
                    stm32_dac_open(&analog, ipc.param1, &de);
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
