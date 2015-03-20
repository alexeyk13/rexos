/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_dac.h"
#include "stm32_power.h"
#include "../../userspace/direct.h"
#include "../../userspace/block.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/sys.h"
#include "../../userspace/file.h"
#include <string.h>
#include "stm32_core_private.h"

#if (DAC_BOFF)
#define DAC_CR_ON                                                           3
#else
#define DAC_CR_ON                                                           1
#endif //DAC_BOFF

#ifdef STM32F1

typedef DMA_Channel_TypeDef* DMA_Channel_TypeDef_P;
static const DMA_Channel_TypeDef_P DAC_DMA_REGS[2] =                        {DMA2_Channel3, DMA2_Channel4};
const unsigned int DAC_DMA_VECTORS[2] =                                     {58, 59};
#define DAC_DMA_NUM                                                         DMA_2
#define DAC_DMA_GLOBAL_REGS                                                 DMA2

static const PIN DAC_PINS[DAC_CHANNELS_COUNT] =                             {A4, A5};
#endif //STM32F1

#ifdef STM32L0
static const PIN DAC_PINS[DAC_CHANNELS_COUNT] =                             {A4};
#endif //STM32L0

#define DAC_TWAKEUP                                                         10


#if (DAC_STREAM)
static inline void stm32_dac_setup_trigger(TIMER_NUM num, int channel)
{
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

static inline void stm32_dac_trigger_open(CORE* core, int num, unsigned int value)
{
    DAC_CHANNEL* dac = core->dac.channels[num];
    switch (dac->flags & DAC_FLAGS_TRIGGER_MASK)
    {
    case DAC_FLAGS_PIN:
        stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_EXTI, dac->pin, dac->flags & 0xf, 0);
        //trigger - EXTI9
        DAC->CR |= (6 << 3) << (16 * num);
        break;
    default:
    }
}

void stm32_dac_dma_isr(int vector, void* param)
{
    DAC_CHANNEL* dac = (DAC_CHANNEL*)param;
    int channel = dac->num == STM32_DAC2 ? 1 : 0;
    DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (8 + 4 * channel);
    dac->half = !dac->half;
    --dac->cnt;

    if (dac->block != INVALID_HANDLE)
    {
        if (dac->cnt >= 2)
        {
            memcpy(dac->fifo + DAC_DMA_FIFO_SIZE * dac->half, dac->ptr, DAC_DMA_FIFO_SIZE);
            dac->ptr += DAC_DMA_FIFO_SIZE;
        }
        else
        {
            fiwrite_complete(dac->process, HAL_HANDLE(HAL_DAC, dac->num), dac->block, dac->size);
            dac->block = INVALID_HANDLE;
        }
    }
    if (dac->cnt <= 0)
    {
        dac->cnt = 0;
        dac->half = 0;
        stm32_dac_trigger_stop(dac);
#if (DAC_DEBUG)
        ipc.process = process_iget_current();
        ipc.cmd = STM32_DAC_UNDERFLOW_DEBUG;
        ipc.param1 = HAL_HANDLE(HAL_DAC, dac->num);
        ipc_ipost(&ipc);
#endif
    }
}
#endif //DAC_STREAM

#if (DAC_STREAM)
static inline void stm32_dac_open(CORE* core, int num, STM32_DAC_TRIGGER trigger)
#else
static inline void stm32_dac_open(CORE* core, int num)
#endif
{
    if (num >= DAC_CHANNELS_COUNT_USER)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (core->dac.channels[num].active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    //enable clock
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    //enable pin
#ifdef STM32F1
    stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, DAC_PINS[num], STM32_GPIO_MODE_INPUT_ANALOG, false);
#if (DAC_DUAL_CHANNEL)
    stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, DAC_PINS[1], STM32_GPIO_MODE_INPUT_ANALOG, false);
#endif //DAC_DUAL_CHANNEL
#endif //STM32F1
#ifdef STM32L0
    stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, DAC_PINS[num], STM32_GPIO_MODE_ANALOG, AF0);
#endif //STM32L0

    DAC->CR |= DAC_CR_ON << (16 * num);
#if (DAC_DUAL_CHANNEL)
    DAC->CR |= DAC_CR_ON << 16;
#endif //DAC_DUAL_CHANNEL

    //TODO: set to 0

#if (DAC_STREAM)
    if (de->flags & DAC_FLAGS_TRIGGER_MASK)
    {
        channel->block = INVALID_HANDLE;
        //DMAEN, TEN
        //TODO: wrong!!!
        DAC->CR |= ((1 << 12) | (1 << 2)) << (16 * num);

        channel->cnt = 0;
        channel->half = 0;

        stm32_power_request_inside(core, STM32_POWER_DMA_ON, DAC_DMA_NUM, 0, 0);
        //TODO: wrong!!!
        DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (8 + 4 * num);

        //register DMA isr
        irq_register(DAC_DMA_VECTORS[num], stm32_dac_dma_isr, (void*)((core->dac.channels[num])));
        NVIC_EnableIRQ(DAC_DMA_VECTORS[num]);
        NVIC_SetPriority(DAC_DMA_VECTORS[num], 10);
        switch (num)
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
        DAC_DMA_REGS[num]->CMAR = (unsigned int)(channel->fifo);
        DAC_DMA_REGS[num]->CNDTR = (DAC_DMA_FIFO_SIZE * 2) / sizeof(uint32_t);
        //High priority, mem: 32, pf: 32, PINC, from memory, TCIE, HCIF, CIRC
        DAC_DMA_REGS[num]->CCR = (2 << 12) | (2 << 10) | (2 << 8) | (1 << 7) | (1 << 5) | (1 << 4) | (1 << 2) | (1 << 1) | (1 << 0);
        stm32_dac_trigger_open(core, num, de->value);
    }
#endif
    sleep_us(DAC_TWAKEUP);
    core->dac.channels[num].active = true;
}

void stm32_dac_close(CORE* core, int num)
{
    if (num >= DAC_CHANNELS_COUNT_USER)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (!core->dac.channels[num].active)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

#if (DAC_STREAM)
    //flush stream, disable trigger
    if (channel->flags & DAC_FLAGS_TRIGGER_MASK)
    {
        stm32_dac_flush(core, num);

        stm32_dac_trigger_close(core, num);
        //disable DMA
        NVIC_DisableIRQ(DAC_DMA_VECTORS[num]);
        //unregister DMA isr
        irq_unregister(DAC_DMA_VECTORS[num]);
        stm32_power_request_inside(core, STM32_POWER_DMA_OFF, DAC_DMA_NUM, 0, 0);
    }
#endif
    //disable channel
    DAC->CR &= ~(0xffff << (16 * num));
#if (DAC_DUAL_CHANNEL)
    DAC->CR &= ~(0xffff << 16);
#endif //DAC_DUAL_CHANNEL

    //turn clock off
    core->dac.channels[num].active = false;
#if (DAC_CHANNELS_COUNT_USER) == 1
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
#else
    if (!core->dac.channels[0].active && !core->dac.channels[1].active)
        RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
#endif //(DAC_CHANNELS_COUNT_USER) == 1

    //disable pin
    stm32_gpio_request_inside(core, STM32_GPIO_DISABLE_PIN, DAC_PINS[num], 0, 0);
#if (DAC_DUAL_CHANNEL)
    stm32_gpio_request_inside(core, STM32_GPIO_DISABLE_PIN, DAC_PINS[1], 0, 0);
#endif //DAC_DUAL_CHANNEL

}

#if (DAC_STREAM)
void stm32_dac_flush(CORE* core, STM32_DAC_TYPE num)
{
    DAC_CHANNEL* channel = core->dac.channels[num];
    stm32_dac_trigger_stop(channel);
    HANDLE block;
    __disable_irq();
    block = channel->block;
    channel->block = INVALID_HANDLE;
    channel->cnt = 0;
    __enable_irq();
    if (block != INVALID_HANDLE)
        block_send(block, channel->process);
}

static inline void stm32_dac_write(CORE* core, STM32_DAC_TYPE num, HANDLE block, unsigned int size, HANDLE process)
{
    if (num >= STM32_DAC_MAX || size == 0)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, ERROR_INVALID_PARAMS);
        return;
    }
    if (core->dac.channels[num] == NULL)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, ERROR_NOT_CONFIGURED);
        return;
    }
    bool need_start = true;
    DAC_CHANNEL* channel = core->dac.channels[num];
    if (channel->cnt > 2)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, ERROR_IN_PROGRESS);
        return;
    }
    if ((channel->ptr = block_open(block)) == NULL)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, get_last_error());
        return;
    }
    channel->block = block;
    channel->process = process;
    channel->size = size;

    unsigned int cnt = channel->size / DAC_DMA_FIFO_SIZE;
    if (channel->size % DAC_DMA_FIFO_SIZE)
        ++cnt;
    unsigned int cnt_left = cnt;
    if (channel->cnt == 0 && cnt >= 2)
    {
        memcpy(channel->fifo, channel->ptr, DAC_DMA_FIFO_SIZE * 2);
        channel->ptr += DAC_DMA_FIFO_SIZE * 2;
        cnt_left -= 2;
    }
    else if (channel->cnt < 2)
    {
        memcpy(channel->fifo + DAC_DMA_FIFO_SIZE * channel->half, channel->ptr, DAC_DMA_FIFO_SIZE);
        channel->ptr += DAC_DMA_FIFO_SIZE;
        --cnt_left;
    }
    if (!cnt_left)
    {
        //ready for next
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, channel->num), channel->block, channel->size);
    }

    __disable_irq();
    need_start = channel->cnt == 0;
    channel->cnt += cnt;
    __enable_irq();

    if (need_start)
        stm32_dac_trigger_start(channel);
}
#endif //DAC_STREAM

void stm32_dac_set_level(CORE* core, int num, int value)
{
    //TODO:
    /*
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
    }*/
}

bool stm32_dac_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_OPEN:
#if (DAC_STREAM)
        stm32_dac_open(core, HAL_ITEM(ipc->param1), ipc->param2);
#else
        stm32_dac_open(core, HAL_ITEM(ipc->param1));
#endif //DAC_STREAM
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_dac_close(core, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case STM32_DAC_SET_LEVEL:
        stm32_dac_set_level(core, HAL_ITEM(ipc->param1), ipc->param2);
        need_post = true;
        break;
#if (DAC_STREAM)
    case IPC_FLUSH:
        stm32_dac_flush(core, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_WRITE:
        stm32_dac_write(core, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //generally posted with block, no return IPC
        break;
#if (DAC_DEBUG)
        //@!@
    case STM32_DAC_UNDERFLOW_DEBUG:
        printd("DAC %d underflow/stop\n\r", HAL_ITEM(ipc->param1));
        //message from isr, no response
        break;
#endif //DAC_DEBUG
#endif //DAC_STREAM
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void stm32_dac_init(CORE* core)
{
    int i;
    for (i = 0; i < DAC_CHANNELS_COUNT_USER; ++i)
        core->dac.channels[i].active = false;
}
