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
#include "../../userspace/object.h"
#include "../sys.h"
#include <string.h>
#include <stdio.h>

#define ADC1_CLOCK_MAX                                  14000000
#define ADC1_TSTAB                                      11
#define ADC_SQR_LEN(len)                                (((len - 1ul) & 0xful) << 20ul)
#define ADC2uV(raw)                                     ((raw * ADC_VREF * 100 / 0xfff) * 10ul)

#define DAC_TWAKEUP                                     10
#define DAC1_PIN                                        A4
#define DAC2_PIN                                        A5
const PIN DAC_OUT[] =                                   {A4, A5};
#if (DAC_DMA)
typedef DMA_Channel_TypeDef* DMA_Channel_TypeDef_P;
static const DMA_Channel_TypeDef_P DAC_DMA_REGS[2] =    {DMA2_Channel3, DMA2_Channel4};
const unsigned int DAC_DMA_VECTORS[2] =                 {58, 59};
#define DAC_DMA_NUM                                     DMA_2
#define DAC_DMA_GLOBAL_REGS                             DMA2

#if (DAC_M2M_DMA)
#define DAC_M2M_DMA_NUM                                 DMA_2
#define DAC_M2M_GLOBAL_REGS                             DMA2
#define DAC_M2M_REGS                                    DMA2_Channel2
#endif

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
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_DRIVERS_IPC_COUNT,
    //function
    stm32_analog
};

typedef struct {
    unsigned int flags;
    PIN pin;
    TIMER_NUM timer;
    HANDLE block, process;
    void* ptr;
    unsigned int size;
    STM32_DAC num;

#if (DAC_DMA)
    HANDLE fifo;
    void* fifo_ptr;
    int cnt;
    int half;
#else
    unsigned int* reg;
    unsigned int processed;
#endif
#if (DAC_DEBUG)
    HANDLE self;
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
static inline void stm32_dac_setup_trigger(TIMER_NUM num, int channel)
{
    //trigger on update
    TIMER_REGS[num]->CR2 = 2 << 4;
    TIMER_REGS[num]->DIER |= TIM_DIER_UDE;
    switch (num)
    {
    case TIM_3:
    case TIM_8:
        DAC->CR |= (1 << 3) << (16 * channel);
        break;
    case TIM_7:
        DAC->CR |= (2 << 3) << (16 * channel);
        break;
    case TIM_5:
        DAC->CR |= (3 << 3) << (16 * channel);
        break;
    case TIM_2:
        DAC->CR |= (4 << 3) << (16 * channel);
        break;
    case TIM_4:
        DAC->CR |= (5 << 3) << (16 * channel);
        break;
    case TIM_6:
    default:
        break;
    }
}
#endif

#if (DAC_DMA) && (DAC_M2M_DMA)
void copy_buf(void* dst, void* src, unsigned int size)
{
    DAC_M2M_REGS->CPAR = (unsigned int)dst;
    DAC_M2M_REGS->CMAR = (unsigned int)src;
    DAC_M2M_REGS->CNDTR = size / sizeof(uint32_t);
    //M2M, very High priority, mem: 32, pf: 32, PINC, MINC, from memory
    DAC_M2M_REGS->CCR = (1 << 14) | (3 << 12) | (2 << 10) | (2 << 8) | (1 << 7) | (1 << 6) | (1 << 4) | (1 << 0);
}
#else
#define copy_buf(dst, src, size)            memcpy(dst, src, size)
#endif

static inline void stm32_dac_trigger_open(ANALOG* analog, int channel, unsigned int value)
{
    DAC_STRUCT* dac = &analog->dac[channel];
    switch (dac->flags & DAC_FLAGS_TRIGGER_MASK)
    {
    case DAC_FLAGS_TIMER:
#if (DAC_DMA)
        ack(analog->core, STM32_TIMER_ENABLE, dac->timer, 0, 0);
        stm32_dac_setup_trigger(dac->timer, channel);
#else
        irq_register(TIMER_VECTORS[dac->timer], stm32_dac_timer_isr, (void*)dac);
        ack(analog->core, STM32_TIMER_ENABLE, dac->timer, TIMER_FLAG_ENABLE_IRQ | (10 << TIMER_FLAG_PRIORITY));
#endif
        ack(analog->core, STM32_TIMER_SETUP_HZ, value, 0, 0);
        break;
    case DAC_FLAGS_PIN:
        ack(analog->core, STM32_GPIO_ENABLE_EXTI, dac->pin, dac->flags & 0xf, 0);
        //trigger - EXTI9
        DAC->CR |= (6 << 3) << (16 * channel);
        break;
    default:
#if (DAC_DMA)
        ack(analog->core, STM32_TIMER_ENABLE_EXT_CLOCK, dac->timer, dac->pin, dac->flags & 0xf);
        stm32_dac_setup_trigger(dac->timer, channel);
#else
        irq_register(TIMER_VECTORS[dac->timer], stm32_dac_timer_isr, (void*)dac);
        ack(analog->core, STM32_TIMER_ENABLE_EXT_CLOCK, dac->timer, dac->pin, (dac->flags & 0xf) | (TIMER_FLAG_ENABLE_IRQ | (10 << TIMER_FLAG_PRIORITY)));
#endif
        TIMER_REGS[dac->timer]->PSC = 0;
        TIMER_REGS[dac->timer]->ARR = value - 1;
    }
}

static inline void stm32_dac_trigger_close(ANALOG* analog, int channel)
{
    DAC_STRUCT* dac = &analog->dac[channel];
    switch (dac->flags & DAC_FLAGS_TRIGGER_MASK)
    {
    case DAC_FLAGS_TIMER:
        ack(analog->core, STM32_TIMER_DISABLE, dac->timer, 0, 0);
        break;
    case DAC_FLAGS_PIN:
        ack(analog->core, STM32_GPIO_DISABLE_EXTI, dac->pin, 0, 0);
        break;
    default:
        ack(analog->core, STM32_TIMER_DISABLE_EXT_CLOCK, dac->timer, dac->pin, 0);
    }
}

static inline void stm32_dac_trigger_start(DAC_STRUCT* dac)
{
    if (dac->flags & DAC_FLAGS_TIMER)
    {
        TIMER_REGS[dac->timer]->CNT = 0;
        TIMER_REGS[dac->timer]->EGR = TIM_EGR_UG;
        TIMER_REGS[dac->timer]->CR1 |= TIM_CR1_CEN;
    }
    if (dac->flags & DAC_FLAGS_PIN)
        EXTI->EMR |= 1ul << 9;
}

static inline void stm32_dac_trigger_stop(DAC_STRUCT* dac)
{
    if (dac->flags & DAC_FLAGS_TIMER)
        TIMER_REGS[dac->timer]->CR1 &= ~TIM_CR1_CEN;
    if (dac->flags & DAC_FLAGS_PIN)
        EXTI->EMR &= ~(1ul << 9);
}

#if (DAC_DMA)
void stm32_dac_dma_isr(int vector, void* param)
{
    IPC ipc;
    DAC_STRUCT* dac = (DAC_STRUCT*)param;
    int channel = dac->num == STM32_DAC2 ? 1 : 0;
    DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (8 + 4 * channel);
    dac->half = !dac->half;
    --dac->cnt;

    if (dac->block != INVALID_HANDLE)
    {
        if (dac->cnt >= 2)
        {
            copy_buf(dac->fifo_ptr + DAC_DMA_FIFO_SIZE * dac->half, dac->ptr, DAC_DMA_FIFO_SIZE);
            dac->ptr += DAC_DMA_FIFO_SIZE;
        }
        else
        {
            ipc.process = dac->process;
            ipc.cmd = IPC_WRITE_COMPLETE;
            ipc.param1 = dac->num;
            ipc.param2 = dac->block;
            ipc.param3 = dac->size;
            block_isend_ipc(dac->block, dac->process, &ipc);
            dac->block = INVALID_HANDLE;
        }
    }
    if (dac->cnt <= 0)
    {
        dac->cnt = 0;
        dac->half = 0;
        stm32_dac_trigger_stop(dac);
#if (DAC_DEBUG)
        ipc.process = dac->self;
        ipc.cmd = STM32_DAC_UNDERFLOW_DEBUG;
        ipc.param1 = dac->num;
        ipc_ipost(&ipc);
#endif
    }
}

#else //DAC_DMA
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

        ipc.process = dac->process;
        ipc.cmd = IPC_WRITE_COMPLETE;
        ipc.param1 = dac->num;
        ipc.param2 = dac->block;
        ipc.param3 = dac->size;
        block_isend_ipc(dac->block, dac->process, &ipc);
        dac->block = INVALID_HANDLE;
    }
    TIMER_REGS[dac->timer]->SR &= ~TIM_SR_UIF;
}
#endif

void stm32_dac_open_channel(ANALOG* analog, int channel)
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

    DAC->CR |= reg << (16 * channel);
}

void stm32_dac_close_channel(ANALOG* analog, int channel)
{
    //DIS
    DAC->CR &= ~(0xffff << (16 * channel));
    //disable PIN
    ack(analog->core, GPIO_DISABLE_PIN, DAC_OUT[channel], 0, 0);
}

void stm32_dac_flush(ANALOG* analog, STM32_DAC num)
{
    IPC ipc;
    int channel = num == STM32_DAC2 ? 1 : 0;
    DAC_STRUCT* dac = &analog->dac[channel];
    stm32_dac_trigger_stop(dac);
    HANDLE block;
    __disable_irq();
    block = dac->block;
    dac->block = INVALID_HANDLE;
#if (DAC_DMA)
    dac->cnt = 0;
#endif
    __enable_irq();
    if (block != INVALID_HANDLE)
    {
        ipc.process = dac->process;
        ipc.cmd = IPC_WRITE_COMPLETE;
        ipc.param1 = num;
        ipc.param2 = block;
        ipc.param3 = dac->size;
        ipc_post(&ipc);
    }
}

void stm32_dac_open(ANALOG* analog, STM32_DAC dac_num, STM32_DAC_ENABLE* de)
{
    //turn clock on
    if (analog->active_channels++ == 0)
        RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    int channel = dac_num == STM32_DAC2 ? 1 : 0;
    DAC_STRUCT* dac = &analog->dac[channel];

    dac->num = dac_num;
    dac->block = INVALID_HANDLE;
    dac->flags = de->flags;
    dac->timer = de->timer;
    dac->pin = de->pin;
    if (dac_num == STM32_DAC1 || dac_num == STM32_DAC_DUAL)
        stm32_dac_open_channel(analog, 0);
    if (dac_num == STM32_DAC2 || dac_num == STM32_DAC_DUAL)
        stm32_dac_open_channel(analog, 1);
#if (DAC_DEBUG)
    dac->self = process_get_current();
#endif
    if (de->flags & DAC_FLAGS_TRIGGER_MASK)
    {
#if (DAC_DMA)
        //DMAEN, TEN
        DAC->CR |= ((1 << 12) | (1 << 2)) << (16 * channel);

        dac->fifo = block_create(DAC_DMA_FIFO_SIZE * 2);
        if (dac->fifo == INVALID_HANDLE)
        {
            if (dac_num == STM32_DAC1 || dac_num == STM32_DAC_DUAL)
                stm32_dac_close_channel(analog, 0);
            if (dac_num == STM32_DAC2 || dac_num == STM32_DAC_DUAL)
                stm32_dac_close_channel(analog, 1);
            return;
        }
        dac->fifo_ptr = block_open(dac->fifo);
        dac->cnt = 0;
        dac->half = 0;

        ack(analog->core, STM32_POWER_DMA_ON, DAC_DMA_NUM, 0, 0);
        DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (8 + 4 * channel);

        //register DMA isr
        irq_register(DAC_DMA_VECTORS[channel], stm32_dac_dma_isr, (void*)(&(analog->dac[channel])));
        NVIC_EnableIRQ(DAC_DMA_VECTORS[channel]);
        NVIC_SetPriority(DAC_DMA_VECTORS[channel], 10);
        switch (dac_num)
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
        DAC_DMA_REGS[channel]->CMAR = (unsigned int)(dac->fifo_ptr);
        DAC_DMA_REGS[channel]->CNDTR = (DAC_DMA_FIFO_SIZE * 2) / sizeof(uint32_t);
        //High priority, mem: 32, pf: 32, PINC, from memory, TCIE, HCIF, CIRC
        DAC_DMA_REGS[channel]->CCR = (2 << 12) | (2 << 10) | (2 << 8) | (1 << 7) | (1 << 5) | (1 << 4) | (1 << 2) | (1 << 1) | (1 << 0);
#else
        switch (dac_num)
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
        stm32_dac_trigger_open(analog, channel, de->value);
    }
    sleep_us(DAC_TWAKEUP);
}

void stm32_dac_close(ANALOG* analog, STM32_DAC num)
{
    int channel = num == STM32_DAC2 ? 1 : 0;
    DAC_STRUCT* dac = &analog->dac[channel];

    if (dac->flags & DAC_FLAGS_TRIGGER_MASK)
    {
        stm32_dac_flush(analog, num);

        stm32_dac_trigger_close(analog, channel);
        //disable DMA
#if (DAC_DMA)
        NVIC_DisableIRQ(DAC_DMA_VECTORS[channel]);
        //unregister DMA isr
        irq_unregister(DAC_DMA_VECTORS[channel]);
        ack(analog->core, STM32_POWER_DMA_OFF, DAC_DMA_NUM, 0, 0);

        block_destroy(analog->dac[channel].fifo);
#endif
    }
    //disable channel
    if (num == STM32_DAC1 || num == STM32_DAC_DUAL)
        stm32_dac_close_channel(analog, 0);
    if (num == STM32_DAC2 || num == STM32_DAC_DUAL)
        stm32_dac_close_channel(analog, 1);
    //turn clock off
    if (--analog->active_channels == 0)
        RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void stm32_dac_write(ANALOG* analog, IPC* ipc)
{
    bool need_start = true;
    if (ipc->param1 > STM32_DAC_DUAL || ipc->param3 == 0)
    {
        ipc_post_error(ipc->process, ERROR_INVALID_PARAMS);
        return;
    }
    int channel = ipc->param1 == STM32_DAC2 ? 1 : 0;
    DAC_STRUCT* dac = &analog->dac[channel];
#if (DAC_DMA)
    if (dac->cnt > 2)
#else
    if (dac->block != INVALID_HANDLE)
#endif
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

#if (DAC_DMA)
    unsigned int cnt = dac->size / DAC_DMA_FIFO_SIZE;
    if (dac->size % DAC_DMA_FIFO_SIZE)
        ++cnt;
    unsigned int cnt_left = cnt;
    if (dac->cnt == 0 && cnt >= 2)
    {
        copy_buf(dac->fifo_ptr, dac->ptr, DAC_DMA_FIFO_SIZE * 2);
        dac->ptr += DAC_DMA_FIFO_SIZE * 2;
        cnt_left -= 2;
    }
    else if (dac->cnt < 2)
    {
        copy_buf(dac->fifo_ptr + DAC_DMA_FIFO_SIZE * dac->half, dac->ptr, DAC_DMA_FIFO_SIZE);
        dac->ptr += DAC_DMA_FIFO_SIZE;
        --cnt_left;
    }
    if (!cnt_left)
    {
        //ready for next
        ipc->cmd = IPC_WRITE_COMPLETE;
        block_send_ipc(dac->block, ipc->process, ipc);
    }

    __disable_irq();
    need_start = dac->cnt == 0;
    dac->cnt += cnt;
    __enable_irq();
#else
    dac->processed = 0;
#endif

    if (need_start)
        stm32_dac_trigger_start(dac);
}

void stm32_dac_set_level(STM32_DAC num, unsigned int value)
{
    switch (num)
    {
    case STM32_DAC1:
        DAC->DHR12R1 = value;
        break;
    case STM32_DAC2:
        DAC->DHR12R2 = value;
        break;
    case STM32_DAC_DUAL:
        DAC->DHR12RD = value;
        break;
    default:
        break;
    }
}

#if (SYS_INFO)
static inline void stm32_analog_info(ANALOG* analog)
{
    printf("STM32 analog driver info\n\r\n\r");
    printf("active channels: %d\n\r", analog->active_channels);
    printf("\n\r\n\r");
}
#endif //SYS_INFO

void stm32_analog()
{
    IPC ipc;
    ANALOG analog;
    STM32_DAC_ENABLE de;
    analog.active_channels = 0;
    analog.core = object_get(SYS_OBJ_CORE);
    if (analog.core == INVALID_HANDLE)
        process_exit();
#if (SYS_INFO)
    open_stdout();
#endif

    for (;;)
    {
        error(ERROR_OK);
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
            stm32_analog_info(&analog);
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
            switch (HAL_GROUP(ipc.param1))
            {
            case HAL_ADC:
                stm32_adc_open(&analog);
                break;
            case HAL_DAC:
                if (direct_read(ipc.process, (void*)&de, sizeof(STM32_DAC_ENABLE)))
                    stm32_dac_open(&analog, ipc.param1, &de);
            default:
                error(ERROR_INVALID_PARAMS);
            }
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
            switch (HAL_GROUP(ipc.param1))
            {
            case HAL_DAC:
                stm32_dac_close(&analog, ipc.param1);
            default:
                error(ERROR_INVALID_PARAMS);
            }
            ipc_post_or_error(&ipc);
            break;
        case STM32_DAC_SET_LEVEL:
            stm32_dac_set_level((STM32_DAC)ipc.param1, ipc.param2);
            ipc_post_or_error(&ipc);
            break;
        case IPC_FLUSH:
            switch (HAL_GROUP(ipc.param1))
            {
            case HAL_DAC:
                stm32_dac_flush(&analog, ipc.param1);
            default:
                error(ERROR_INVALID_PARAMS);
            }
            ipc_post_or_error(&ipc);
            break;
        case IPC_WRITE:
            stm32_dac_write(&analog, &ipc);
            //generally posted with block, no return IPC
            break;
#if (DAC_DEBUG)
        case STM32_DAC_UNDERFLOW_DEBUG:
            printf("DAC %d underflow/stop\n\r", ipc.param1);
            break;
#endif
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
