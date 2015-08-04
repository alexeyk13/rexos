/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_dac.h"
#include "stm32_power.h"
#include "stm32_timer.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/sys.h"
#include "../../userspace/htimer.h"
#include <string.h>
#include "stm32_core_private.h"
#include "../wavegen.h"

#if (DAC_BOFF)
#define DAC_CR_ON                                                           3
#else
#define DAC_CR_ON                                                           1
#endif //DAC_BOFF

typedef DMA_Channel_TypeDef* DMA_Channel_TypeDef_P;

#ifdef STM32F1


#define DAC_DMA                                                             DMA_2
#define DAC_DMA_GLOBAL_REGS                                                 DMA2
static const DMA_Channel_TypeDef_P DAC_DMA_REGS[DAC_CHANNELS_COUNT] =       {DMA2_Channel3, DMA2_Channel4};
static const STM32_DMA_CHANNEL DAC_DMA_CHANNELS[DAC_CHANNELS_COUNT] =       {DMA_CHANNEL_3, DMA_CHANNEL_4};
const unsigned int DAC_DMA_VECTORS[DAC_CHANNELS_COUNT] =                    {58, 59};
static const PIN DAC_PINS[DAC_CHANNELS_COUNT] =                             {A4, A5};
static const TIMER_NUM DAC_TRIGGERS[DAC_CHANNELS_COUNT] =                   {TIM_6, TIM_7};
#if (DAC_DUAL_CHANNEL)
static const unsigned int DAC_DATA_REG[DAC_CHANNELS_COUNT_USER] =           {DAC_BASE + 0x20};
#else
static const unsigned int DAC_DATA_REG[DAC_CHANNELS_COUNT_USER] =           {DAC_BASE + 0x08, DAC_BASE + 0x14};
#endif //DAC_DUAL_CHANNEL

//In F1 STM defined bits with differrent names for every DMA channel.
#define DMA_CCR_MINC                                                        (1 << 7)
#define DMA_CCR_CIRC                                                        (1 << 5)
#define DMA_CCR_DIR                                                         (1 << 4)
#define DMA_CCR_HTIE                                                        (1 << 2)
#define DMA_CCR_TCIE                                                        (1 << 1)
#define DMA_CCR_EN                                                          (1 << 0)
#endif //STM32F1

#ifdef STM32L0
#define DAC_DMA                                                             DMA_1
#define DAC_DMA_GLOBAL_REGS                                                 DMA1
static const DMA_Channel_TypeDef_P DAC_DMA_REGS[DAC_CHANNELS_COUNT] =       {DMA1_Channel2};
static const STM32_DMA_CHANNEL DAC_DMA_CHANNELS[DAC_CHANNELS_COUNT] =       {DMA_CHANNEL_2};
const unsigned int DAC_DMA_VECTORS[DAC_CHANNELS_COUNT] =                    {10};
static const PIN DAC_PINS[DAC_CHANNELS_COUNT] =                             {A4};
static const TIMER_NUM DAC_TRIGGERS[DAC_CHANNELS_COUNT] =                   {TIM_6};
static const unsigned int DAC_DATA_REG[DAC_CHANNELS_COUNT] =       {DAC_BASE + 0x08};
#endif //STM32L0

#define DAC_TWAKEUP                                                         10

#if (DAC_STREAM)
#define HALF_FIFO_BYTES                                                     ((DAC_DMA_FIFO_SIZE >> 1) * sizeof(SAMPLE))

void stm32_dac_dma_isr(int vector, void* param)
{
    CORE* core = (CORE*)param;
#if (DAC_MANY)
    int num = vector == DAC_DMA_VECTORS[0] ? 0 : 1;
#else
    int num = 0;
#endif //DAC_MANY
    DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (DAC_DMA_CHANNELS[num] << 2);
    core->dac.channels[num].half = !core->dac.channels[num].half;
    --core->dac.channels[num].cnt;

    if (core->dac.channels[num].io != NULL)
    {
        if (core->dac.channels[num].cnt >= 2)
        {
            memcpy(core->dac.channels[num].fifo + HALF_FIFO_BYTES * core->dac.channels[num].half, core->dac.channels[num].ptr, HALF_FIFO_BYTES);
            core->dac.channels[num].ptr += HALF_FIFO_BYTES;
        }
        else
        {
            iio_complete(core->dac.channels[num].process, HAL_IO_CMD(HAL_DAC, IPC_WRITE), num, core->dac.channels[num].io);
            core->dac.channels[num].io = NULL;
        }
    }
    if (core->dac.channels[num].cnt <= 0)
    {
        core->dac.channels[num].half = 0;
        stm32_timer_request_inside(core, HAL_CMD(HAL_TIMER, TIMER_STOP), DAC_TRIGGERS[num], 0, 0);
#if (DAC_DEBUG)
        IPC ipc;
        ipc.process = process_iget_current();
        ipc.cmd = HAL_CMD(HAL_DAC, STM32_DAC_UNDERFLOW_DEBUG);
        ipc.param1 = num;
        ipc_ipost(&ipc);
#endif
    }
}
#endif //DAC_STREAM

static inline void stm32_dac_open(CORE* core, int num, DAC_MODE mode, unsigned int samplerate)
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
    stm32_pin_request_inside(core, HAL_CMD(HAL_PIN, STM32_GPIO_ENABLE_PIN), DAC_PINS[num], STM32_GPIO_MODE_INPUT_ANALOG, false);
#if (DAC_DUAL_CHANNEL)
    stm32_pin_request_inside(core, HAL_CMD(HAL_PIN, STM32_GPIO_ENABLE_PIN), DAC_PINS[1], STM32_GPIO_MODE_INPUT_ANALOG, false);
#endif //DAC_DUAL_CHANNEL
#endif //STM32F1
#ifdef STM32L0
    stm32_pin_request_inside(core, HAL_CMD(HAL_PIN, STM32_GPIO_ENABLE_PIN), DAC_PINS[num], STM32_GPIO_MODE_ANALOG, AF0);
#endif //STM32L0

    DAC->CR = 0;

    //setup trigger
    if (mode != DAC_MODE_LEVEL)
    {
        core->dac.channels[num].fifo = malloc(DAC_DMA_FIFO_SIZE * sizeof(SAMPLE));
        if (core->dac.channels[num].fifo == NULL)
            return;

        core->dac.channels[num].samplerate = samplerate;
        stm32_timer_request_inside(core, HAL_CMD(HAL_TIMER, IPC_OPEN), DAC_TRIGGERS[num], STM32_TIMER_DMA_ENABLE, 0);
        DAC->CR |= DAC_CR_DMAEN1 << (16 * num);

        //setup DMA
        stm32_power_request_inside(core, HAL_CMD(HAL_POWER, STM32_POWER_SET_CLOCK_SOURCE), STM32_CLOCK_SOURCE_DMA, DAC_DMA, true);
        DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (DAC_DMA_CHANNELS[num] << 2);

        //dst
        DAC_DMA_REGS[num]->CPAR = DAC_DATA_REG[num];
        //src
        DAC_DMA_REGS[num]->CMAR = (unsigned int)(core->dac.channels[num].fifo);
        DAC_DMA_REGS[num]->CNDTR = DAC_DMA_FIFO_SIZE;
#if (DAC_DUAL_CHANNEL)
        //High priority, mem: 32, pf: 32
        DAC_DMA_REGS[num]->CCR = (2 << 12) | (2 << 10) | (2 << 8) | DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_DIR | DMA_CCR_EN;
#else
        //High priority, mem: 16, pf: 32
        DAC_DMA_REGS[num]->CCR = (2 << 12) | (1 << 10) | (2 << 8) | DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_DIR | DMA_CCR_EN;
#endif
#ifdef STM32L0
        //map channel 2 to DAC
        DMA1_CSELR->CSELR &= ~(0xf << 4);
        DMA1_CSELR->CSELR |= (0x9 << 4);
#endif //STM32L0

#if (DAC_STREAM)
        if (mode != DAC_MODE_WAVE)
        {
            //register DMA isr
            DAC_DMA_REGS[num]->CCR |= DMA_CCR_HTIE | DMA_CCR_TCIE;
            irq_register(DAC_DMA_VECTORS[num], stm32_dac_dma_isr, core);
            NVIC_EnableIRQ(DAC_DMA_VECTORS[num]);
            NVIC_SetPriority(DAC_DMA_VECTORS[num], 10);
            core->dac.channels[num].io = NULL;
            core->dac.channels[num].cnt = 0;
            core->dac.channels[num].half = 0;
        }
#endif //DAC_STREAM
    }

    DAC->CR |= DAC_CR_ON << (16 * num);
#if (DAC_DUAL_CHANNEL)
    DAC->CR |= DAC_CR_ON << 16;
#endif //DAC_DUAL_CHANNEL

    sleep_us(DAC_TWAKEUP);

    if (mode == DAC_MODE_LEVEL)
        *(unsigned int*)(DAC_DATA_REG[num]) = 0;

    core->dac.channels[num].mode = mode;
    core->dac.channels[num].active = true;
}

#if (DAC_STREAM)
static void stm32_dac_flush(CORE* core, int num)
{
    IO* io;
    stm32_timer_request_inside(core, HAL_CMD(HAL_TIMER, TIMER_STOP), DAC_TRIGGERS[num], 0, 0);
    __disable_irq();
    io = core->dac.channels[num].io;
    core->dac.channels[num].io = NULL;
    __enable_irq();
    if (io != NULL)
        io_complete_ex(core->dac.channels[num].process, HAL_IO_CMD(HAL_DAC, IPC_WRITE), num, io, ERROR_IO_CANCELLED);
}
#endif //DAC_STREAM

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

    if (core->dac.channels[num].mode != DAC_MODE_LEVEL)
    {
#if (DAC_STREAM)
        if (core->dac.channels[num].mode != DAC_MODE_WAVE)
        {
            stm32_dac_flush(core, num);
            irq_unregister(DAC_DMA_VECTORS[num]);
        }
#endif //DAC_STREAM
        NVIC_DisableIRQ(DAC_DMA_VECTORS[num]);
        stm32_power_request_inside(core, HAL_CMD(HAL_TIMER, STM32_POWER_SET_CLOCK_SOURCE), STM32_CLOCK_SOURCE_DMA, DAC_DMA, false);
        stm32_timer_request_inside(core, HAL_CMD(HAL_TIMER, TIMER_STOP), DAC_TRIGGERS[num], 0, 0);
    }

    //disable channel
    DAC->CR &= ~(0xffff << (16 * num));
#if (DAC_DUAL_CHANNEL)
    DAC->CR &= ~(0xffff << 16);
#endif //DAC_DUAL_CHANNEL

    //turn clock off
    core->dac.channels[num].active = false;
#if (DAC_MANY)
    if (!core->dac.channels[0].active && !core->dac.channels[1].active)
        RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
#else
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
#endif //(DAC_MANY)

    //disable pin
    stm32_pin_request_inside(core, HAL_CMD(HAL_PIN, STM32_GPIO_DISABLE_PIN), DAC_PINS[num], 0, 0);
#if (DAC_DUAL_CHANNEL)
    stm32_pin_request_inside(core, HAL_CMD(HAL_PIN, STM32_GPIO_DISABLE_PIN), DAC_PINS[1], 0, 0);
#endif //DAC_DUAL_CHANNEL
}

#if (DAC_STREAM)
static inline void stm32_dac_write(CORE* core, IPC*ipc)
{
    unsigned int num = ipc->param1;
    bool need_start = true;
    if (num >= DAC_CHANNELS_COUNT_USER)
    {
        ipc_post_ex(ipc, ERROR_INVALID_PARAMS);
        return;
    }
    if (!core->dac.channels[num].active)
    {
        ipc_post_ex(ipc, ERROR_NOT_CONFIGURED);
        return;
    }
    if (core->dac.channels[num].cnt > 2)
    {
        ipc_post_ex(ipc, ERROR_IN_PROGRESS);
        return;
    }
    core->dac.channels[num].io = (IO*)ipc->param2;
    core->dac.channels[num].process = ipc->process;
    core->dac.channels[num].size = ipc->param3;
    core->dac.channels[num].ptr = io_data(core->dac.channels[num].io);

    unsigned int cnt = core->dac.channels[num].size / HALF_FIFO_BYTES;
    //unaligned data will be ignored
    unsigned int cnt_left = cnt;
    if (core->dac.channels[num].cnt == 0 && cnt >= 2)
    {
        memcpy(core->dac.channels[num].fifo, core->dac.channels[num].ptr, HALF_FIFO_BYTES * 2);
        core->dac.channels[num].ptr += HALF_FIFO_BYTES * 2;
        cnt_left -= 2;
    }
    else if (core->dac.channels[num].cnt < 2)
    {
        memcpy(core->dac.channels[num].fifo + HALF_FIFO_BYTES * core->dac.channels[num].half, core->dac.channels[num].ptr, HALF_FIFO_BYTES);
        core->dac.channels[num].ptr += HALF_FIFO_BYTES;
        --cnt_left;
    }
    if (!cnt_left)
    {
        //ready for next
        ipc_post(ipc);
    }

    __disable_irq();
    need_start = core->dac.channels[num].cnt == 0;
    core->dac.channels[num].cnt += cnt;
    __enable_irq();

    if (need_start)
        stm32_timer_request_inside(core, HAL_CMD(HAL_TIMER, TIMER_START), DAC_TRIGGERS[num], TIMER_VALUE_HZ, core->dac.channels[num].samplerate);
}
#endif //DAC_STREAM

void stm32_dac_set_level(CORE* core, int num, int value)
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
    *(unsigned int*)(DAC_DATA_REG[num]) = 0;
}

void stm32_dac_wave(CORE* core, int num, DAC_WAVE_TYPE wave_type, int amplitude)
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
    stm32_timer_request_inside(core, HAL_CMD(HAL_TIMER, TIMER_STOP), DAC_TRIGGERS[num], 0, 0);
    if (amplitude)
    {
        wave_gen(core->dac.channels[num].fifo, DAC_DMA_FIFO_SIZE, wave_type, amplitude);
        stm32_timer_request_inside(core, HAL_CMD(HAL_TIMER, TIMER_START), DAC_TRIGGERS[num], TIMER_VALUE_HZ, core->dac.channels[num].samplerate * DAC_DMA_FIFO_SIZE);
    }
}

bool stm32_dac_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_dac_open(core, ipc->param1, (DAC_MODE)ipc->param2, ipc->param3);
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_dac_close(core, ipc->param1);
        need_post = true;
        break;
    case DAC_SET:
        stm32_dac_set_level(core, ipc->param1, ipc->param2);
        need_post = true;
        break;
    case DAC_WAVE:
        stm32_dac_wave(core, ipc->param1, (DAC_WAVE_TYPE)ipc->param2, ipc->param3);
        need_post = true;
        break;
#if (DAC_STREAM)
    case IPC_FLUSH:
        stm32_dac_flush(core, ipc->param1);
        need_post = true;
        break;
    case IPC_WRITE:
        stm32_dac_write(core, ipc);
        //posted with io, no return IPC
        break;
#if (DAC_DEBUG)
    case STM32_DAC_UNDERFLOW_DEBUG:
        printd("DAC %d underflow/stop\n", ipc->param1);
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
    {
        core->dac.channels[i].fifo = NULL;
        core->dac.channels[i].active = false;
    }
}
