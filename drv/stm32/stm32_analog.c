/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_analog.h"
#include "stm32_power.h"
#include "stm32_config.h"
#include "../../userspace/direct.h"
#include "../../userspace/block.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/sys.h"
#include "../../userspace/file.h"
#include <string.h>
#if (MONOLITH_ANALOG)
#include "stm32_core_private.h"
#endif

#if (STM32_DAC)
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
#endif //STM32_DAC

#if (MONOLITH_ANALOG)

#define get_clock               stm32_power_get_clock_inside
#define ack_gpio                stm32_gpio_request_inside
#define ack_power               stm32_power_request_inside
#define ack_timer               stm32_timer_request_inside

#else

#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside
#define ack_power               stm32_core_request_outside
#define ack_timer               stm32_core_request_outside

void stm32_analog();

const REX __STM32_ANALOG = {
    //name
    "STM32 analog",
    //size
    STM32_ANALOG_STACK_SIZE,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_DRIVERS_IPC_COUNT,
    //function
    stm32_analog
};

#endif

#if (STM32_DAC)

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

static inline void stm32_dac_trigger_open(SHARED_ANALOG_DRV* drv, int channel, unsigned int value)
{
    DAC_STRUCT* dac = drv->analog.dac[channel];
    switch (dac->flags & DAC_FLAGS_TRIGGER_MASK)
    {
    case DAC_FLAGS_TIMER:
#if (DAC_DMA)
        ack_timer(drv, STM32_TIMER_ENABLE, dac->timer, 0, 0);
        stm32_dac_setup_trigger(dac->timer, channel);
#else
        irq_register(TIMER_VECTORS[dac->timer], stm32_dac_timer_isr, (void*)dac);
        ack_timer(drv, STM32_TIMER_ENABLE, dac->timer, TIMER_FLAG_ENABLE_IRQ | (10 << TIMER_FLAG_PRIORITY));
#endif
        ack_timer(drv, STM32_TIMER_SETUP_HZ, value, 0, 0);
        break;
    case DAC_FLAGS_PIN:
        ack_gpio(drv, STM32_GPIO_ENABLE_EXTI, dac->pin, dac->flags & 0xf, 0);
        //trigger - EXTI9
        DAC->CR |= (6 << 3) << (16 * channel);
        break;
    default:
#if (DAC_DMA)
        ack_timer(drv, STM32_TIMER_ENABLE_EXT_CLOCK, dac->timer, dac->pin, dac->flags & 0xf);
        stm32_dac_setup_trigger(dac->timer, channel);
#else
        irq_register(TIMER_VECTORS[dac->timer], stm32_dac_timer_isr, (void*)dac);
        ack_timer(drv, STM32_TIMER_ENABLE_EXT_CLOCK, dac->timer, dac->pin, (dac->flags & 0xf) | (TIMER_FLAG_ENABLE_IRQ | (10 << TIMER_FLAG_PRIORITY)));
#endif
        TIMER_REGS[dac->timer]->PSC = 0;
        TIMER_REGS[dac->timer]->ARR = value - 1;
    }
}

static inline void stm32_dac_trigger_close(SHARED_ANALOG_DRV* drv, int channel)
{
    DAC_STRUCT* dac = drv->analog.dac[channel];
    switch (dac->flags & DAC_FLAGS_TRIGGER_MASK)
    {
    case DAC_FLAGS_TIMER:
        ack_timer(drv, STM32_TIMER_DISABLE, dac->timer, 0, 0);
        break;
    case DAC_FLAGS_PIN:
        ack_gpio(drv, STM32_GPIO_DISABLE_EXTI, dac->pin, 0, 0);
        break;
    default:
        ack_timer(drv, STM32_TIMER_DISABLE_EXT_CLOCK, dac->timer, dac->pin, 0);
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
    DAC_STRUCT* dac = (DAC_STRUCT*)param;
    int channel = dac->num == STM32_DAC2 ? 1 : 0;
    DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (8 + 4 * channel);
    dac->half = !dac->half;
    --dac->cnt;

    if (dac->block != INVALID_HANDLE)
    {
        if (dac->cnt >= 2)
        {
            copy_buf(dac->fifo + DAC_DMA_FIFO_SIZE * dac->half, dac->ptr, DAC_DMA_FIFO_SIZE);
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

#else //DAC_DMA
void stm32_dac_timer_isr(int vector, void* param)
{
    IPC ipc;
    DAC_STRUCT* dac = (DAC_STRUCT*)param;
    switch (dac->num)
    {
    case STM32_DAC1:
        DAC->DHR12R1 = *((unsigned int*)(dac->ptr));
        break;
    case STM32_DAC2:
        DAC->DHR12R2 = *((unsigned int*)(dac->ptr));
        break;
    case STM32_DAC_DUAL:
        DAC->DHR12RD = *((unsigned int*)(dac->ptr));
        break;
    default:
        break;
    }
    dac->ptr += sizeof(unsigned int);
    dac->processed += sizeof(unsigned int);

    if (dac->processed >= dac->size)
    {
        TIMER_REGS[dac->timer]->CR1 &= ~TIM_CR1_CEN;

        ipc.process = dac->process;
        ipc.cmd = IPC_WRITE_COMPLETE;
        ipc.param1 = HAL_HANDLE(HAL_DAC, dac->num);
        ipc.param2 = dac->block;
        ipc.param3 = dac->size;
        block_isend_ipc(dac->block, dac->process, &ipc);
        dac->block = INVALID_HANDLE;
    }
    TIMER_REGS[dac->timer]->SR &= ~TIM_SR_UIF;
}
#endif

void stm32_dac_open_channel(SHARED_ANALOG_DRV* drv, int channel)
{
    uint32_t reg;
    //enable PIN analog mode
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, DAC_OUT[channel], STM32_GPIO_MODE_INPUT_ANALOG, false);
    DAC->CR &= ~(0xffff << (16 * channel));
    //EN
    reg = (1 << 0);
#if (DAC_BOFF)
    reg |= (1 << 1);
#endif

    DAC->CR |= reg << (16 * channel);
}

void stm32_dac_close_channel(SHARED_ANALOG_DRV* drv, int channel)
{
    //DIS
    DAC->CR &= ~(0xffff << (16 * channel));
    //disable PIN
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, DAC_OUT[channel], 0, 0);
}

void stm32_dac_flush(SHARED_ANALOG_DRV* drv, STM32_DAC_TYPE num)
{
    int channel = num == STM32_DAC2 ? 1 : 0;
    DAC_STRUCT* dac = drv->analog.dac[channel];
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
        block_send(block, dac->process);
}

static inline void stm32_dac_open_internal(SHARED_ANALOG_DRV* drv, STM32_DAC_TYPE dac_num, STM32_DAC_ENABLE* de)
{
    if (dac_num >= STM32_DAC_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    int channel = dac_num == STM32_DAC2 ? 1 : 0;
    if (drv->analog.dac[0] == NULL && drv->analog.dac[1] == NULL)
        RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    if (drv->analog.dac[channel] != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    DAC_STRUCT* dac = drv->analog.dac[channel] = malloc(sizeof(DAC_STRUCT));
    if (dac == NULL)
        return;


    dac->num = dac_num;
    dac->block = INVALID_HANDLE;
    dac->flags = de->flags;
    dac->timer = de->timer;
    dac->pin = de->pin;
    if (dac_num == STM32_DAC1 || dac_num == STM32_DAC_DUAL)
        stm32_dac_open_channel(drv, 0);
    if (dac_num == STM32_DAC2 || dac_num == STM32_DAC_DUAL)
        stm32_dac_open_channel(drv, 1);
    if (de->flags & DAC_FLAGS_TRIGGER_MASK)
    {
#if (DAC_DMA)
        //DMAEN, TEN
        DAC->CR |= ((1 << 12) | (1 << 2)) << (16 * channel);

        dac->cnt = 0;
        dac->half = 0;

        ack_power(drv, STM32_POWER_DMA_ON, DAC_DMA_NUM, 0, 0);
        DAC_DMA_GLOBAL_REGS->IFCR |= 0xf << (8 + 4 * channel);

        //register DMA isr
        irq_register(DAC_DMA_VECTORS[channel], stm32_dac_dma_isr, (void*)((drv->analog.dac[channel])));
        NVIC_EnableIRQ(DAC_DMA_VECTORS[channel]);
        NVIC_SetPriority(DAC_DMA_VECTORS[channel], 10);
        switch (dac_num)
        {
        case STM32_DAC1:
            DAC_DMA_REGS[0]->CPAR = (unsigned inhttp://www.rbc.ru/t)&(DAC->DHR12R1);
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
        DAC_DMA_REGS[channel]->CMAR = (unsigned int)(dac->fifo);
        DAC_DMA_REGS[channel]->CNDTR = (DAC_DMA_FIFO_SIZE * 2) / sizeof(uint32_t);
        //High priority, mem: 32, pf: 32, PINC, from memory, TCIE, HCIF, CIRC
        DAC_DMA_REGS[channel]->CCR = (2 << 12) | (2 << 10) | (2 << 8) | (1 << 7) | (1 << 5) | (1 << 4) | (1 << 2) | (1 << 1) | (1 << 0);
#endif
        stm32_dac_trigger_open(drv, channel, de->value);
    }
    sleep_us(DAC_TWAKEUP);
}

void stm32_dac_open(SHARED_ANALOG_DRV* drv, STM32_DAC_TYPE dac_num, HANDLE process)
{
    STM32_DAC_ENABLE de;
    if (direct_read(process, (void*)&de, sizeof(STM32_DAC_ENABLE)))
        stm32_dac_open_internal(drv, dac_num, &de);
}

void stm32_dac_close(SHARED_ANALOG_DRV* drv, STM32_DAC_TYPE num)
{
    if (num >= STM32_DAC_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    int channel = num == STM32_DAC2 ? 1 : 0;

    if (drv->analog.dac[channel] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    DAC_STRUCT* dac = drv->analog.dac[channel];

    if (dac->flags & DAC_FLAGS_TRIGGER_MASK)
    {
        stm32_dac_flush(drv, num);

        stm32_dac_trigger_close(drv, channel);
        //disable DMA
#if (DAC_DMA)
        NVIC_DisableIRQ(DAC_DMA_VECTORS[channel]);
        //unregister DMA isr
        irq_unregister(DAC_DMA_VECTORS[channel]);
        ack_power(drv, STM32_POWER_DMA_OFF, DAC_DMA_NUM, 0, 0);
#endif
    }
    //disable channel
    if (num == STM32_DAC1 || num == STM32_DAC_DUAL)
        stm32_dac_close_channel(drv, 0);
    if (num == STM32_DAC2 || num == STM32_DAC_DUAL)
        stm32_dac_close_channel(drv, 1);
    free(drv->analog.dac[channel]);
    drv->analog.dac[channel] = NULL;
    //turn clock off
    if (drv->analog.dac[0] == NULL && drv->analog.dac[1] == NULL)
        RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

static inline void stm32_dac_write(SHARED_ANALOG_DRV* drv, STM32_DAC_TYPE num, HANDLE block, unsigned int size, HANDLE process)
{
    if (num >= STM32_DAC_MAX || size == 0)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, ERROR_INVALID_PARAMS);
        return;
    }
    int channel = num == STM32_DAC2 ? 1 : 0;

    if (drv->analog.dac[channel] == NULL)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, ERROR_NOT_CONFIGURED);
        return;
    }
    bool need_start = true;
    DAC_STRUCT* dac = drv->analog.dac[channel];
#if (DAC_DMA)
    if (dac->cnt > 2)
#else
    if (dac->block != INVALID_HANDLE)
#endif
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, ERROR_IN_PROGRESS);
        return;
    }
    if ((dac->ptr = block_open(block)) == NULL)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, num), block, get_last_error());
        return;
    }
    dac->block = block;
    dac->process = process;
    dac->size = size;

#if (DAC_DMA)
    unsigned int cnt = dac->size / DAC_DMA_FIFO_SIZE;
    if (dac->size % DAC_DMA_FIFO_SIZE)
        ++cnt;
    unsigned int cnt_left = cnt;
    if (dac->cnt == 0 && cnt >= 2)
    {
        copy_buf(dac->fifo, dac->ptr, DAC_DMA_FIFO_SIZE * 2);
        dac->ptr += DAC_DMA_FIFO_SIZE * 2;
        cnt_left -= 2;
    }
    else if (dac->cnt < 2)
    {
        copy_buf(dac->fifo + DAC_DMA_FIFO_SIZE * dac->half, dac->ptr, DAC_DMA_FIFO_SIZE);
        dac->ptr += DAC_DMA_FIFO_SIZE;
        --cnt_left;
    }
    if (!cnt_left)
    {
        //ready for next
        fwrite_complete(process, HAL_HANDLE(HAL_DAC, dac->num), dac->block, dac->size);
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

void stm32_dac_set_level(STM32_DAC_TYPE num, unsigned int value)
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

#endif //STM32_DAC

bool stm32_analog_request(SHARED_ANALOG_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_OPEN:
        switch (HAL_GROUP(ipc->param1))
        {
#if (STM32_DAC)
        case HAL_DAC:
            stm32_dac_open(drv, HAL_ITEM(ipc->param1), ipc->process);
#endif //STM32_DAC
        default:
            error(ERROR_INVALID_PARAMS);
        }
        need_post = true;
        break;
    case IPC_CLOSE:
        switch (HAL_GROUP(ipc->param1))
        {
#if (STM32_DAC)
        case HAL_DAC:
            stm32_dac_close(drv, HAL_ITEM(ipc->param1));
#endif //STM32_DAC
        default:
            error(ERROR_INVALID_PARAMS);
        }
        need_post = true;
        break;
#if (STM32_DAC)
    case STM32_DAC_SET_LEVEL:
        stm32_dac_set_level((STM32_DAC_TYPE)HAL_ITEM(ipc->param1), ipc->param2);
        need_post = true;
        break;
    case IPC_FLUSH:
        switch (HAL_GROUP(ipc->param1))
        {
        case HAL_DAC:
            stm32_dac_flush(drv, HAL_ITEM(ipc->param1));
        default:
            error(ERROR_INVALID_PARAMS);
        }
        need_post = true;
        break;
    case IPC_WRITE:
        stm32_dac_write(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //generally posted with block, no return IPC
        break;
#if (DAC_DEBUG)
    case STM32_DAC_UNDERFLOW_DEBUG:
        printf("DAC %d underflow/stop\n\r", HAL_ITEM(ipc->param1));
        //message from isr, no response
        break;
#endif
#endif //STM32_DAC
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void stm32_analog_init(SHARED_ANALOG_DRV* drv)
{
    drv->analog.dac[0] = drv->analog.dac[1] = NULL;
}

#if !(MONOLITH_ANALOG)
void stm32_analog()
{
    IPC ipc;
    SHARED_ANALOG_DRV drv;
    bool need_post;
    stm32_analog_init(&drv);
    object_set_self(SYS_OBJ_ANALOG);
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        case IPC_SET_STDIO:
            open_stdout();
            need_post = true;
            break;
        default:
            need_post = stm32_analog_request(&drv, &ipc);
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
#endif
