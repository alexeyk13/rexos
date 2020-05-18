/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/

#include "nrf_pwm.h"
#include "../../userspace/io.h"
#include "../kerror.h"
#include "nrf_exo_private.h"
#include <string.h>

#define PWM_MAX_CNT_VALUE                  ((1 << 15)-1)
#define PWM_INVERSE_POLARITY_MASK          (1 << 15)
#define PWM_FREQ_HZ                        16000000

typedef NRF_PWM_Type*                       NRF_PWM_Type_P;

const int PWM_VECTORS[PWMS_COUNT]         = {PWM0_IRQn, PWM1_IRQn, PWM2_IRQn};
const NRF_PWM_Type_P PWM_REGS[PWMS_COUNT] = {NRF_PWM0, NRF_PWM1, NRF_PWM2};


static inline void nrf_pwm_open(EXO* exo, TIMER_NUM num, unsigned int flags)
{
    NRF_PWM_Type_P base=PWM_REGS[num];
    exo->timer.pwm.flags[num] = flags;
    // keep flags for channel enable/disable
//    exo->timer.flags[num] = flags;

    base->TASKS_STOP = 1;
    base->ENABLE = PWM_ENABLE_ENABLE_Msk;
    base->SHORTS = PWM_SHORTS_LOOPSDONE_SEQSTART0_Msk;
    base->MODE = PWM_MODE_UPDOWN_Up;
    base->DECODER = (PWM_DECODER_LOAD_Individual << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
    base->LOOP = 1;

    base->SEQ[0].PTR = base->SEQ[1].PTR = (uint32_t)(&exo->timer.pwm.seq[num]);
    base->SEQ[0].CNT = base->SEQ[1].CNT = PWM_CHANNELS;
    base->SEQ[0].REFRESH = base->SEQ[1].REFRESH = 0;
    base->SEQ[0].ENDDELAY = base->SEQ[1].ENDDELAY = 0;

    if (flags & TIMER_IRQ_ENABLE)
    {
        base->INTENSET = PWM_INTENSET_LOOPSDONE_Msk;
        NVIC_EnableIRQ(PWM_VECTORS[num]);
        NVIC_SetPriority(PWM_VECTORS[num], 13/*TIMER_IRQ_PRIORITY_VALUE(flags)*/);
    }
}

static inline void nrf_pwm_close(EXO* exo, TIMER_NUM num)
{
    PWM_REGS[num]->TASKS_STOP = 1;
    NVIC_DisableIRQ(PWM_VECTORS[num]);
    PWM_REGS[num]->ENABLE = 0;
}

static inline void nrf_pwm_setup_channel(EXO* exo, int num, int channel, TIMER_CHANNEL_TYPE type, unsigned int value)
{
    value &= (1 << 15) - 1;
    value |= PWM_INVERSE_POLARITY_MASK;
    switch (type)
    {
    case TIMER_CHANNEL_DISABLE:
        value = 0;
        break;
    case TIMER_CHANNEL_PWM_INVERSE_POLARITY:
        value &= ~PWM_INVERSE_POLARITY_MASK;
        break;
    default:
        break;
    }
    exo->timer.pwm.seq[num].ch_duty[channel] = value;
/*
    if(exo->timer.flags[num] & TIMER_IRQ_ENABLE)
        TIMER_REGS[num]->INTENSET |= ((TIMER_INTENSET_COMPARE0_Set << cc_num) << TIMER_INTENSET_COMPARE0_Pos);
*/
}
static inline void nrf_pwm_set_duty(EXO* exo, int num, int channel, TIMER_VALUE_TYPE type, unsigned int value)
{
    uint32_t top = PWM_REGS[num]->COUNTERTOP;
    switch(type)
    {
    case TIMER_VALUE_DUTY_PERCENT:
        value *= 10;
    case TIMER_VALUE_DUTY_PROMILLE:
        value = value * top;
        value = value  / 500;
        value = (value + 1) >> 1;
        break;
    default:
        break;
    }
    exo->timer.pwm.seq[num].ch_duty[channel] = (exo->timer.pwm.seq[num].ch_duty[channel] & PWM_INVERSE_POLARITY_MASK) | (value & PWM_MAX_CNT_VALUE);
}


static void nrf_pwm_setup_clk(NRF_PWM_Type_P base, uint32_t clk)
{
    uint8_t div;
    for(div = 0; div < 7; div++, clk >>= 1)
        if(clk <= PWM_MAX_CNT_VALUE)
            break;

    if(clk > PWM_MAX_CNT_VALUE)
        clk = PWM_MAX_CNT_VALUE;
    base->COUNTERTOP = clk;
    base->PRESCALER = div;

}

static inline void nrf_pwm_start(EXO* exo, TIMER_NUM num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    NRF_PWM_Type_P base=PWM_REGS[num];
    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        nrf_pwm_setup_clk(base, PWM_FREQ_HZ/value);
        break;
    case TIMER_VALUE_US:
        nrf_pwm_setup_clk(base, (PWM_FREQ_HZ/1000000)*value);
        break;
    default:
        nrf_pwm_setup_clk(base, value);
    }

    base->TASKS_SEQSTART[0] = 1;
}

static inline void nrf_pwm_stop(TIMER_NUM num)
{
    PWM_REGS[num]->TASKS_STOP = 1;
}

void nrf_pwm_init(EXO* exo)
{
    memset(&exo->timer.pwm, 0, sizeof(PWM_DRV));
}

void nrf_pwm_request(EXO* exo, IPC* ipc)
{
    TIMER_NUM num = (TIMER_NUM)ipc->param1;
    if (num >= PWM_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    num -= PWM_0;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        nrf_pwm_open(exo, num, ipc->param2);
        break;
    case IPC_CLOSE:
        nrf_pwm_close(exo, num);
        break;
    case TIMER_START:
        nrf_pwm_start(exo, num, (TIMER_VALUE_TYPE)ipc->param2, ipc->param3);
        break;
    case TIMER_STOP:
        nrf_pwm_stop(num);
        break;
    case TIMER_SETUP_CHANNEL:
        nrf_pwm_setup_channel(exo, num, TIMER_CHANNEL_VALUE(ipc->param2), TIMER_CHANNEL_TYPE_VALUE(ipc->param2), ipc->param3);
        break;
    case TIMER_SET_DUTY:
        nrf_pwm_set_duty(exo, num, TIMER_CHANNEL_VALUE(ipc->param2), TIMER_VAUE_TYPE_VALUE(ipc->param2), ipc->param3);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}
