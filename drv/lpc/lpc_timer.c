/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_timer.h"
#include "lpc_core_private.h"
#include "lpc_power.h"
#include "lpc_config.h"
#include "../../userspace/systime.h"
#include "../../userspace/irq.h"

#define TC16PC                                      15
#define S1_US                                       1000000

#ifdef LPC11Uxx
static const uint8_t __TIMER_POWER_PINS[] =         {SYSCON_SYSAHBCLKCTRL_CT16B0_POS, SYSCON_SYSAHBCLKCTRL_CT16B1_POS,
                                                     SYSCON_SYSAHBCLKCTRL_CT32B0_POS, SYSCON_SYSAHBCLKCTRL_CT32B1_POS};
static const uint8_t __TIMER_VECTORS[] =            {16, 17, 18, 19};

typedef LPC_CTxxBx_Type* LPC_CTxxBx_Type_P;

static const LPC_CTxxBx_Type_P __TIMER_REGS[] =     {LPC_CT16B0, LPC_CT16B1, LPC_CT32B0, LPC_CT32B1};

void lpc_timer_isr(int vector, void* param)
{
    if ((__TIMER_REGS[SECOND_TIMER]->IR & (1 << HPET_CHANNEL)) && (__TIMER_REGS[SECOND_TIMER]->MCR & (1 << (HPET_CHANNEL * 3))))
    {
        __TIMER_REGS[SECOND_TIMER]->IR = 1 << HPET_CHANNEL;
        systime_hpet_timeout();
    }
    if (__TIMER_REGS[SECOND_TIMER]->IR & (1 << SECOND_CHANNEL))
    {
        __TIMER_REGS[SECOND_TIMER]->IR = (1 << SECOND_CHANNEL);
        systime_second_pulse();
    }
}

void lpc_timer_open(CORE* core, TIMER timer, unsigned int flags)
{
    core->timer.main_channel[timer] = (flags & TIMER_MODE_CHANNEL_MASK) >> TIMER_MODE_CHANNEL_POS;

    //enable clock
    LPC_SYSCON->SYSAHBCLKCTRL |= 1 << __TIMER_POWER_PINS[timer];

    if (flags & TIMER_IRQ_ENABLE)
    {
        //clear pending interrupts
        NVIC_ClearPendingIRQ(__TIMER_VECTORS[timer]);
        if (timer & 1)
            __TIMER_REGS[timer]->IR = CT_IR_MR0INT | CT_IR_MR1INT | CT_IR_MR2INT | CT_IR_MR3INT | CT_IR_CR0INT | CT1_IR_CR1INT;
        else
            __TIMER_REGS[timer]->IR = CT_IR_MR0INT | CT_IR_MR1INT | CT_IR_MR2INT | CT_IR_MR3INT | CT_IR_CR0INT | CT0_IR_CR1INT;
        //enable interrupts
        NVIC_EnableIRQ(__TIMER_VECTORS[timer]);
        NVIC_SetPriority(__TIMER_VECTORS[timer], TIMER_IRQ_PRIORITY_VALUE(flags));
    }

    if (flags & TIMER_ONE_PULSE)
        __TIMER_REGS[timer]->MCR |= (CT_MCR_MR0I | CT_MCR_MR0S) << (core->timer.main_channel[timer] * 3);
    else
        __TIMER_REGS[timer]->MCR |= (CT_MCR_MR0I | CT_MCR_MR0R) << (core->timer.main_channel[timer] * 3);
}

void lpc_timer_close(CORE* core, TIMER timer)
{
    core->timer.main_channel[timer] = TIMER_CHANNEL_INVALID;
    //disable interrupts
    NVIC_DisableIRQ(__TIMER_VECTORS[timer]);
    //disable clock
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << __TIMER_POWER_PINS[timer]);
}


static void lpc_timer_start_master_clk(CORE* core, TIMER timer, unsigned int psc, unsigned int mr)
{
    __TIMER_REGS[timer]->TC = __TIMER_REGS[timer]->PC = 0;
    __TIMER_REGS[timer]->PR = psc - 1;
    __TIMER_REGS[timer]->MR[core->timer.main_channel[timer]] = mr - 1;
    //start counter
    __TIMER_REGS[timer]->TCR |= CT_TCR_CEN;
}

static void lpc_timer_start_master_us(CORE* core, TIMER timer, unsigned int us)
{
    unsigned int psc, cnt, clk;
    unsigned int clock = lpc_power_get_core_clock_inside(core);
    //for 32 bit timers routine is different and much more easy
    if (timer >= TC32B0)
        //psc is always 1us
        lpc_timer_start_master_clk(core, timer, clock / 1000000, us);
    else
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
        lpc_timer_start_master_clk(core, timer, psc, cnt);
    }
}

void lpc_timer_start(CORE* core, TIMER timer, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        lpc_timer_start_master_us(core, timer, 1000000 / value);
        break;
    case TIMER_VALUE_US:
        lpc_timer_start_master_us(core, timer, value);
        break;
    case TIMER_VALUE_CLK:
        lpc_timer_start_master_clk(core, timer, 1, value);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void lpc_timer_setup_channel(CORE* core, TIMER timer, int channel, TIMER_CHANNEL_TYPE type, unsigned int value)
{
    unsigned int match;
    switch (type)
    {
    case TIMER_CHANNEL_GENERAL:
        match = __TIMER_REGS[timer]->TC + (value > __TIMER_REGS[timer]->MR[core->timer.main_channel[timer]] ?
                                                                __TIMER_REGS[timer]->MR[core->timer.main_channel[timer]] : value);
        if (match > __TIMER_REGS[timer]->MR[core->timer.main_channel[timer]])
            match -= (__TIMER_REGS[timer]->MR[core->timer.main_channel[timer]] + 1);
         __TIMER_REGS[timer]->MR[channel] = match;
        //enable interrupt on match channel
        __TIMER_REGS[timer]->MCR |= 1 << (channel * 3);
        break;
    case TIMER_CHANNEL_DISABLE:
        __TIMER_REGS[timer]->MCR &= ~(7 << (channel * 3));
        //clear pending match interrupt
        __TIMER_REGS[timer]->IR = 1 << channel;
        break;
    case TIMER_CHANNEL_OUTPUT_PWM_FALL:
        //enable PWM on channel
        __TIMER_REGS[timer]->PWMC |= 1 << channel;
        //update value
        __TIMER_REGS[timer]->MR[channel] = value;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void lpc_timer_stop(CORE* core, TIMER timer)
{
    __TIMER_REGS[timer]->TCR &= ~CT_TCR_CEN;
    //clear pending match interrupt
    __TIMER_REGS[timer]->IR = 0xf;
}

void hpet_start(unsigned int value, void* param)
{
    CORE* core = (CORE*)param;
    core->timer.hpet_start = __TIMER_REGS[SECOND_TIMER]->TC;
    //don't need to start in free-run mode, second pulse will go faster anyway
    if (value < S1_US)
       lpc_timer_setup_channel(core, SECOND_TIMER, HPET_CHANNEL, TIMER_CHANNEL_GENERAL, SECOND_TIMER >= TC32B0 ? value : value / TC16PC);
}

void hpet_stop(void* param)
{
    CORE* core = (CORE*)param;
    lpc_timer_setup_channel(core, SECOND_TIMER, HPET_CHANNEL, TIMER_CHANNEL_DISABLE, 0);
}

unsigned int hpet_elapsed(void* param)
{
    CORE* core = (CORE*)param;
    unsigned int tc = __TIMER_REGS[SECOND_TIMER]->TC;
    unsigned int value = core->timer.hpet_start < tc ? tc - core->timer.hpet_start : __TIMER_REGS[SECOND_TIMER]->MR[HPET_CHANNEL] + 1 - core->timer.hpet_start + tc;
    return SECOND_TIMER >= TC32B0 ? value : value / TC16PC;
}
#endif //LPC11Uxx

void lpc_timer_init(CORE *core)
{
#ifdef LPC11Uxx
    int i;
    core->timer.hpet_start = 0;
    for (i = 0; i < TIMER_MAX; ++i)
        core->timer.main_channel[i] = TIMER_CHANNEL_INVALID;

    //setup second tick timer
    irq_register(__TIMER_VECTORS[SECOND_TIMER], lpc_timer_isr, (void*)core);
    lpc_timer_open(core, SECOND_TIMER, TIMER_IRQ_ENABLE | (SECOND_CHANNEL << TIMER_MODE_CHANNEL_POS) | (2 << TIMER_IRQ_PRIORITY_POS));
    lpc_timer_start(core, SECOND_TIMER, TIMER_VALUE_US, S1_US);

    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    systime_hpet_setup(&cb_svc_timer, core);
#endif //LPC11Uxx
}

bool lpc_timer_request(CORE* core, IPC* ipc)
{
    TIMER timer = (TIMER)ipc->param1;
    if (timer >= TIMER_MAX)
    {
        error(ERROR_NOT_SUPPORTED);
        return true;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
#ifdef LPC11Uxx
    case IPC_OPEN:
        lpc_timer_open(core, timer, ipc->param2);
        break;
    case IPC_CLOSE:
        lpc_timer_close(core, timer);
        break;
    case TIMER_START:
        lpc_timer_start(core, timer, ipc->param2, ipc->param3);
        break;
    case TIMER_STOP:
        lpc_timer_stop(core, timer);
        break;
    case TIMER_SETUP_CHANNEL:
        lpc_timer_setup_channel(core, timer, TIMER_CHANNEL_VALUE(ipc->param2), TIMER_CHANNEL_TYPE_VALUE(ipc->param2), ipc->param3);
        break;
#endif //LPC11Uxx
    default:
        error(ERROR_NOT_SUPPORTED);
    }
    return true;
}
