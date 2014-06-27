/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "core_kernel_cortexm.h"
#include "kernel_config.h"
#include "../../kernel.h"
#include "../../thread_kernel.h"
#include "../../dbg.h"

#define PSP_IN_LR                                   0xfffffffd
#define SVC_12                                      0xdf12

#define CALLER_ADDRESS                              6


#if (KERNEL_PROFILING)
//TODO: move to dbg
unsigned int stack_used_max(unsigned int top, unsigned int cur)
{
    unsigned int i;
    unsigned int last = cur;
    for (i = cur - sizeof(unsigned int); i >= top; i -= 4)
        if (*(unsigned int*)i != MAGIC_UNINITIALIZED)
            last = i;
    return last;
}

void svc_stack_stat()
{
/*    unsigned int sp, current_stack, max_stack;
    printf("    type       stack        top\n\r");
    printf("-----------------------------------\n\r");
    sp = (unsigned int)__get_MSP();
    current_stack = (SVC_STACK_END - sp) / sizeof(unsigned int);
    max_stack = (SVC_STACK_END - stack_used_max(SVC_STACK_TOP, sp)) / sizeof(unsigned int);
    printf("Supervisor %3d/%3d/%3d   0x%08x\n\r", current_stack, max_stack, SVC_STACK_SIZE, SVC_STACK_TOP);*/
}
#endif //KERNEL_PROFILING

static void process_fault(unsigned int ret_value)
{
    //from thread context, just killing thread
    if (ret_value == PSP_IN_LR)
    {
        svc_thread_destroy_current();
        SCB_CFSR = 0;
    }
    else
        panic();
}

void on_hard_fault(unsigned int ret_value, unsigned int* stack_value)
{
#if (KERNEL_DEBUG)
//        printf("HARD FAULT: ");
        if (SCB_HFSR & HFSR_VECTTBL)
        {
        }
///            printf("Vector table read fault at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
        //wrong sys call
        else if (*(uint16_t*)(stack_value[CALLER_ADDRESS] - 2) == SVC_12)
        {
//            printf("SYS call while disabled interrupts at %#.08x\n\r", stack_value[5] & 0xfffffffe);
//            gpio_enable_pin(GPIO_B9, PIN_MODE_OUT);
//            gpio_set_pin(GPIO_B9, true);
        }
        else
        {
        }
//            printf("General hard fault at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
#endif
    if (ret_value == PSP_IN_LR && (*(uint16_t*)(stack_value[CALLER_ADDRESS] - 2) == SVC_12))
        __ASM volatile ("cpsie i");
    process_fault(ret_value);
}

void on_mem_manage(unsigned int ret_value, unsigned int* stack_value)
{
    if (SCB_CFSR & (CFSR_MSTKERR | CFSR_MUNSTKERR))
    {
#if (KERNEL_DEBUG)
        printf("MEM MANAGE: ");
        if (SCB_CFSR & CFSR_MSTKERR)
            printf("Stacking failed");
        else if (SCB_CFSR & CFSR_MUNSTKERR)
            printf("Unstacking failed");
        printf(" at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
#endif
        panic();
    }
    else if (SCB_CFSR & (CFSR_DACCVIOL | CFSR_IACCVIOL))
    {
#if (KERNEL_DEBUG)
        printf("MEM MANAGE: ");
        if (SCB_CFSR & CFSR_DACCVIOL)
            printf("Data access violation");
        else if (SCB_CFSR & CFSR_IACCVIOL)
            printf("Instruction access violation");
        printf(" at %#.08x\n\r", SCB_MMAR);
#endif
        process_fault(ret_value);
    }
    else
        on_hard_fault(ret_value, stack_value);
}

void on_bus_fault(unsigned int ret_value, unsigned int* stack_value)
{
    if (SCB_CFSR & (CFSR_BSTKERR | CFSR_BUNSTKERR | CFSR_IMPRECISERR | CFSR_PRECISERR | CFSR_IBUSERR))
    {
#if (KERNEL_DEBUG)
        printf("BUS FAULT: ");
        if (SCB_CFSR & CFSR_BSTKERR)
            printf("Stacking failed at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
        else if (SCB_CFSR & CFSR_BUNSTKERR)
            printf("Unstacking failed at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
        else if (SCB_CFSR & (CFSR_IMPRECISERR | CFSR_PRECISERR))
            printf("Data bus error at %#.08x\n\r", SCB_BFAR);
        else if (SCB_CFSR & CFSR_IBUSERR)
            printf("Instruction bus error at %#.08x\n\r", SCB_BFAR);
#endif
        panic();
    }
    else
        on_hard_fault(ret_value, stack_value);
}

void on_usage_fault(unsigned int ret_value, unsigned int* stack_value)
{
#if (KERNEL_DEBUG)
    printf("USAGE FAULT: ");
    if (SCB_CFSR & CFSR_DIVBYZERO)
        printf("Division by zero");
    else if (SCB_CFSR & CFSR_UNALIGNED)
        printf("Unaligned access");
    else if (SCB_CFSR & CFSR_NOCP)
        printf("No coprocessor found");
    else if (SCB_CFSR & (CFSR_UNDEFINSTR | CFSR_INVPC))
        printf("Undefined instruction");
    else if (SCB_CFSR & CFSR_INVSTATE)
        printf("Invalid state");
    printf(" at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
#endif

    process_fault(ret_value);
}
