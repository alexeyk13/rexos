/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "core_kernel_cortexm.h"
#include "kernel_config.h"
#include "../../kernel.h"
#include "../../svc_process.h"
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
    printk("    type       stack        top\n\r");
    printk("-----------------------------------\n\r");
    sp = (unsigned int)__get_MSP();
    current_stack = (SVC_STACK_END - sp) / sizeof(unsigned int);
    max_stack = (SVC_STACK_END - stack_used_max(SVC_STACK_TOP, sp)) / sizeof(unsigned int);
    printk("Supervisor %3d/%3d/%3d   0x%08x\n\r", current_stack, max_stack, SVC_STACK_SIZE, SVC_STACK_TOP);*/
}
#endif //KERNEL_PROFILING

static void process_fault(unsigned int ret_value)
{
    //from thread context, just killing thread
    if (ret_value == PSP_IN_LR)
    {
        svc_process_destroy_current();
        SCB_CFSR = 0;
    }
    else
        panic();
}

void on_hard_fault(unsigned int ret_value, unsigned int* stack_value)
{
//    gpio_set_pin(GPIO_B9, true);
#if (KERNEL_DEBUG)
///        printk("HARD FAULT: ");
        if (SCB_HFSR & HFSR_VECTTBL)
        {
//            gpio_set_pin(GPIO_B9, true);
        }
//            printk("Vector table read fault at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
        //wrong sys call
        else if (*(uint16_t*)(stack_value[CALLER_ADDRESS] - 2) == SVC_12)
        {
//            gpio_set_pin(GPIO_B9, true);
        }
//            printk("SYS call while disabled interrupts at %#.08x\n\r", stack_value[5] & 0xfffffffe);
        else
            printk("General hard fault at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
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
        printk("MEM MANAGE: ");
        if (SCB_CFSR & CFSR_MSTKERR)
            printk("Stacking failed");
        else if (SCB_CFSR & CFSR_MUNSTKERR)
            printk("Unstacking failed");
        printk(" at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
#endif
        panic();
    }
    else if (SCB_CFSR & (CFSR_DACCVIOL | CFSR_IACCVIOL))
    {
#if (KERNEL_DEBUG)
        printk("MEM MANAGE: ");
        if (SCB_CFSR & CFSR_DACCVIOL)
            printk("Data access violation");
        else if (SCB_CFSR & CFSR_IACCVIOL)
            printk("Instruction access violation");
        printk(" at %#.08x\n\r", SCB_MMAR);
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
        printk("BUS FAULT: ");
        if (SCB_CFSR & CFSR_BSTKERR)
            printk("Stacking failed at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
        else if (SCB_CFSR & CFSR_BUNSTKERR)
            printk("Unstacking failed at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
        else if (SCB_CFSR & (CFSR_IMPRECISERR | CFSR_PRECISERR))
            printk("Data bus error at %#.08x\n\r", SCB_BFAR);
        else if (SCB_CFSR & CFSR_IBUSERR)
            printk("Instruction bus error at %#.08x\n\r", SCB_BFAR);
#endif
        panic();
    }
    else
        on_hard_fault(ret_value, stack_value);
}

void on_usage_fault(unsigned int ret_value, unsigned int* stack_value)
{
#if (KERNEL_DEBUG)
    printk("USAGE FAULT: ");
    if (SCB_CFSR & CFSR_DIVBYZERO)
        printk("Division by zero");
    else if (SCB_CFSR & CFSR_UNALIGNED)
        printk("Unaligned access");
    else if (SCB_CFSR & CFSR_NOCP)
        printk("No coprocessor found");
    else if (SCB_CFSR & (CFSR_UNDEFINSTR | CFSR_INVPC))
        printk("Undefined instruction");
    else if (SCB_CFSR & CFSR_INVSTATE)
        printk("Invalid state");
    printk(" at %#.08x\n\r", stack_value[CALLER_ADDRESS]);
#endif

    process_fault(ret_value);
}
