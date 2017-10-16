/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_timer.h"
#include "lpc_exo_private.h"
#include "lpc_power.h"
#include "lpc_config.h"
#include "../ksystime.h"
#include "../kirq.h"
#include "../kerror.h"
#include "../../userspace/systime.h"

#define S1_US                                       1000000

#ifdef LPC11Uxx
#define TC16PC                                      15
static const uint8_t __TIMER_POWER_PINS[] =         {SYSCON_SYSAHBCLKCTRL_CT16B0_POS, SYSCON_SYSAHBCLKCTRL_CT16B1_POS,
                                                     SYSCON_SYSAHBCLKCTRL_CT32B0_POS, SYSCON_SYSAHBCLKCTRL_CT32B1_POS};
typedef LPC_CTxxBx_Type* LPC_CTxxBx_Type_P;
static const LPC_CTxxBx_Type_P __TIMER_REGS[] =     {LPC_CT16B0, LPC_CT16B1, LPC_CT32B0, LPC_CT32B1};
static const uint8_t __TIMER_VECTORS[] =            {16, 17, 18, 19};
#else //LPC18xx
typedef LPC_TIMERn_Type* LPC_TIMERn_Type_P;
static const LPC_TIMERn_Type_P __TIMER_REGS[] =     {LPC_TIMER0, LPC_TIMER1, LPC_TIMER2, LPC_TIMER3};
static const uint8_t __TIMER_VECTORS[] =            {12, 13, 14, 15};
#endif //LPC11Uxx


void lpc_timer_isr(int vector, void* param)
{
    if ((__TIMER_REGS[SECOND_TIMER]->IR & (1 << HPET_CHANNEL)) && (__TIMER_REGS[SECOND_TIMER]->MCR & (1 << (HPET_CHANNEL * 3))))
    {
        __TIMER_REGS[SECOND_TIMER]->IR = 1 << HPET_CHANNEL;
        ksystime_hpet_timeout();
    }
    if (__TIMER_REGS[SECOND_TIMER]->IR & (1 << SECOND_CHANNEL))
    {
        __TIMER_REGS[SECOND_TIMER]->IR = (1 << SECOND_CHANNEL);
        ksystime_second_pulse();
    }
}

void lpc_timer_open(EXO* exo, TIMER timer, unsigned int flags)
{
    unsigned int channel = (flags & TIMER_MODE_CHANNEL_MASK) >> TIMER_MODE_CHANNEL_POS;
    exo->timer.main_channel[timer] = channel;
#ifdef LPC18xx
    if (timer >= PWM0)
        //no special setup is required
        return;
    if (timer >= SCT)
    {
        //unified
        LPC_SCT->CONFIG = SCT_CONFIG_UNIFY_Msk;
        //channel used as limit
        LPC_SCT->LIMIT_L = 1 << channel;
        //enabled in all states
        LPC_SCT->EVENT[channel].STATE = 0xffff;
        LPC_SCT->EVENT[channel].CTRL = (channel << SCT_EVCTRL0_MATCHSEL_Pos) | (1 << SCT_EVCTRL0_COMBMODE_Pos);
        return;
    }
#endif //LPC18xx

    //enable clock
#ifdef LPC11Uxx
    LPC_SYSCON->SYSAHBCLKCTRL |= 1 << __TIMER_POWER_PINS[timer];
#endif //LPC11Uxx

    if (flags & TIMER_IRQ_ENABLE)
    {
        //clear pending interrupts
        NVIC_ClearPendingIRQ(__TIMER_VECTORS[timer]);
        __TIMER_REGS[timer]->IR = 0xf;
        //enable interrupts
        NVIC_EnableIRQ(__TIMER_VECTORS[timer]);
        NVIC_SetPriority(__TIMER_VECTORS[timer], TIMER_IRQ_PRIORITY_VALUE(flags));
    }

    if (flags & TIMER_ONE_PULSE)
        __TIMER_REGS[timer]->MCR |= (TIMER0_MCR_MR0I_Msk | TIMER0_MCR_MR0S_Msk) << (exo->timer.main_channel[timer] * 3);
    else
        __TIMER_REGS[timer]->MCR |= (TIMER0_MCR_MR0I_Msk | TIMER0_MCR_MR0R_Msk) << (exo->timer.main_channel[timer] * 3);
}

void lpc_timer_close(EXO* exo, TIMER timer)
{
    exo->timer.main_channel[timer] = TIMER_CHANNEL_INVALID;
#ifdef LPC18xx
    if (timer >= SCT)
        //no special setup is required
        return;
#endif //LPC18xx
    //disable interrupts
    NVIC_DisableIRQ(__TIMER_VECTORS[timer]);
#ifdef LPC11Uxx
    //disable clock
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << __TIMER_POWER_PINS[timer]);
#endif //LPC11Uxx
}

static void lpc_timer_start_master_clk(EXO* exo, TIMER timer, unsigned int psc, unsigned int mr)
{
    __TIMER_REGS[timer]->TC = __TIMER_REGS[timer]->PC = 0;
    __TIMER_REGS[timer]->PR = psc - 1;
    __TIMER_REGS[timer]->MR[exo->timer.main_channel[timer]] = mr - 1;
    //start counter
    __TIMER_REGS[timer]->TCR |= TIMER0_TCR_CEN_Msk;
}

static void lpc_timer_start_master_us(EXO* exo, TIMER timer, unsigned int us)
{
    //timers operate on M3 clock
    unsigned int clock = lpc_power_get_core_clock_inside();
#ifdef LPC11Uxx
    unsigned int psc, cnt, clk;
    if (timer < TC32B0)
    {
        clk = clock / 1000000 * us;
        //convert value to clock units
        psc = clk / 0x8000;
        if (!psc)
            psc = 1;
        cnt = clk / psc;
        if (cnt < 2)
            cnt = 2;
        if (cnt > 0x10000)
            cnt = 0x10000;
        lpc_timer_start_master_clk(exo, timer, psc, cnt);
    }
    else
#endif //LPC11Uxx
    {
        //for 32 bit timers routine is different and much more easy - psc is always 1us
        lpc_timer_start_master_clk(exo, timer, clock / 1000000, us);
    }
}

void lpc_timer_start(EXO* exo, TIMER timer, TIMER_VALUE_TYPE value_type, unsigned int value)
{
#ifdef LPC18xx
    if (timer >= SCT)
    {
        if (value_type != TIMER_VALUE_CLK)
        {
            kerror(ERROR_NOT_SUPPORTED);
            return;
        }
        //set value for channel

        if (timer >= PWM0)
        {
            LPC_MCPWM->TC[timer - PWM0] = 0;
            LPC_MCPWM->LIM[timer - PWM0] = value - 1;
            LPC_MCPWM->CON_SET = MCPWM_CON_RUN0_Msk << (8 * (timer - PWM0));
            return;
        }
        //SCT
        LPC_SCT->MATCHREL[exo->timer.main_channel[timer]].L = value - 1;
        //unhalt
        LPC_SCT->CTRL_L &= ~SCT_CTRL_HALT_L_Msk;
        return;
    }
#endif //LPC18xx
    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        lpc_timer_start_master_us(exo, timer, 1000000 / value);
        break;
    case TIMER_VALUE_US:
        lpc_timer_start_master_us(exo, timer, value);
        break;
    case TIMER_VALUE_CLK:
        lpc_timer_start_master_clk(exo, timer, 1, value);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

static inline void lpc_timer_setup_channel(EXO* exo, TIMER timer, int channel, TIMER_CHANNEL_TYPE type, unsigned int value)
{
    unsigned int match;
#ifdef LPC18xx
    if (timer >= SCT)
    {
        if (type != TIMER_CHANNEL_PWM)
        {
            kerror(ERROR_NOT_SUPPORTED);
            return;
        }

        if (timer >= PWM0)
        {
            LPC_MCPWM->MAT[timer - PWM0] = value;
            LPC_MCPWM->CON_CLR = MCPWM_CON_POLA0_Msk << (8 * (timer - PWM0));
            return;
        }
        //SCT
        //enable channel
        LPC_SCT->EVENT[channel].STATE = 0xffff;
        LPC_SCT->EVENT[channel].CTRL = (channel << SCT_EVCTRL0_MATCHSEL_Pos) | (1 << SCT_EVCTRL0_COMBMODE_Pos);
        //set match value
        LPC_SCT->MATCHREL[channel].L = value;
        //set on channel output
        LPC_SCT->OUT[channel].SET = (1 << channel);
        //master channel will reset pin
        LPC_SCT->OUT[channel].CLR = (1 << exo->timer.main_channel[timer]);
        return;
    }
#endif //LPC18xx
    switch (type)
    {
    case TIMER_CHANNEL_GENERAL:
        match = __TIMER_REGS[timer]->TC + (value > __TIMER_REGS[timer]->MR[exo->timer.main_channel[timer]] ?
                                                                __TIMER_REGS[timer]->MR[exo->timer.main_channel[timer]] : value);
        if (match > __TIMER_REGS[timer]->MR[exo->timer.main_channel[timer]])
            match -= (__TIMER_REGS[timer]->MR[exo->timer.main_channel[timer]] + 1);
         __TIMER_REGS[timer]->MR[channel] = match;
        //enable interrupt on match channel
        __TIMER_REGS[timer]->MCR |= 1 << (channel * 3);
        break;
    case TIMER_CHANNEL_DISABLE:
        __TIMER_REGS[timer]->MCR &= ~(7 << (channel * 3));
        //clear pending match interrupt
        __TIMER_REGS[timer]->IR = 1 << channel;
        break;
#ifdef LPC11Uxx
    case TIMER_CHANNEL_PWM:
        //enable PWM on channel
        __TIMER_REGS[timer]->PWMC |= 1 << channel;
        //update value
        __TIMER_REGS[timer]->MR[channel] = value;
        break;
#endif //LPC11Uxx
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

void lpc_timer_stop(EXO* exo, TIMER timer)
{
#ifdef LPC18xx
    if (timer >= PWM0)
    {
        LPC_MCPWM->CON_CLR = MCPWM_CON_RUN0_Msk << (8 * (timer - PWM0));
        return;
    }
    if (timer >= SCT)
    {
        //halt
        LPC_SCT->CTRL_L |= SCT_CTRL_HALT_L_Msk;
    }
#endif //LPC18xx
    __TIMER_REGS[timer]->TCR &= ~TIMER0_TCR_CEN_Msk;
    //clear pending match interrupt
    __TIMER_REGS[timer]->IR = 0xf;
}

void hpet_start(unsigned int value, void* param)
{
    EXO* exo = (EXO*)param;
    exo->timer.hpet_start = __TIMER_REGS[SECOND_TIMER]->TC;
    //don't need to start in free-run mode, second pulse will go faster anyway
    if (value < S1_US)
    {
#ifdef LPC11Uxx
        if (SECOND_TIMER < TC32B0)
            lpc_timer_setup_channel(exo, SECOND_TIMER, HPET_CHANNEL, TIMER_CHANNEL_GENERAL, value / TC16PC);
        else
#endif //LPC11Uxx
            lpc_timer_setup_channel(exo, SECOND_TIMER, HPET_CHANNEL, TIMER_CHANNEL_GENERAL, value);
    }
}

void hpet_stop(void* param)
{
    EXO* exo = (EXO*)param;
    lpc_timer_setup_channel(exo, SECOND_TIMER, HPET_CHANNEL, TIMER_CHANNEL_DISABLE, 0);
}

unsigned int hpet_elapsed(void* param)
{
    EXO* exo = (EXO*)param;
    unsigned int tc = __TIMER_REGS[SECOND_TIMER]->TC;
    unsigned int value = exo->timer.hpet_start < tc ? tc - exo->timer.hpet_start : __TIMER_REGS[SECOND_TIMER]->MR[HPET_CHANNEL] + 1 - exo->timer.hpet_start + tc;
#ifdef LPC11Uxx
    if (SECOND_TIMER < TC32B0)
        return value / TC16PC;
    else
#endif //LPC11Uxx
        return value;
}

void lpc_timer_init(EXO* exo)
{
    int i;
    exo->timer.hpet_start = 0;
    for (i = 0; i < TIMER_MAX; ++i)
        exo->timer.main_channel[i] = TIMER_CHANNEL_INVALID;

    //setup second tick timer
    kirq_register(KERNEL_HANDLE, __TIMER_VECTORS[SECOND_TIMER], lpc_timer_isr, (void*)exo);
    lpc_timer_open(exo, SECOND_TIMER, TIMER_IRQ_ENABLE | (SECOND_CHANNEL << TIMER_MODE_CHANNEL_POS) | (2 << TIMER_IRQ_PRIORITY_POS));
    lpc_timer_start(exo, SECOND_TIMER, TIMER_VALUE_US, S1_US);

    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    ksystime_hpet_setup(&cb_svc_timer, exo);
}

bool lpc_timer_request(EXO* exo, IPC* ipc)
{
    TIMER timer = (TIMER)ipc->param1;
    if (timer >= TIMER_MAX)
    {
        kerror(ERROR_NOT_SUPPORTED);
        return true;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_timer_open(exo, timer, ipc->param2);
        break;
    case IPC_CLOSE:
        lpc_timer_close(exo, timer);
        break;
    case TIMER_START:
        lpc_timer_start(exo, timer, ipc->param2, ipc->param3);
        break;
    case TIMER_STOP:
        lpc_timer_stop(exo, timer);
        break;
    case TIMER_SETUP_CHANNEL:
        lpc_timer_setup_channel(exo, timer, TIMER_CHANNEL_VALUE(ipc->param2), TIMER_CHANNEL_TYPE_VALUE(ipc->param2), ipc->param3);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
    return true;
}

#if (POWER_MANAGEMENT)
void lpc_timer_suspend(EXO* exo)
{
    __TIMER_REGS[SECOND_TIMER]->TCR &= ~TIMER0_TCR_CEN_Msk;
}

void lpc_timer_adjust(EXO* exo)
{
    __TIMER_REGS[SECOND_TIMER]->PR = (lpc_power_get_core_clock_inside() / S1_US) - 1;
    __TIMER_REGS[SECOND_TIMER]->TCR |= TIMER0_TCR_CEN_Msk;
}
#endif //(POWER_MANAGEMENT)
