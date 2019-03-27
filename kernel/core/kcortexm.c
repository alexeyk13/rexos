/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "kcortexm.h"
#include "kernel_config.h"
#include "../kernel.h"
#include "../kprocess.h"
#include "../dbg.h"

#define PSP_IN_LR                                   0xfffffffd
#define SVC_12                                      0xdf12

#define LR_IN_STACK                                 5
#define PC_IN_STACK                                 6
#define LAZY_STACKING_CONTEXT_SIZE                  8

#if (KERNEL_DEBUG)
static const char* const __REG_NAMES[LAZY_STACKING_CONTEXT_SIZE] =           {"R0", "R1", "R2", "R3", "R12", "LR", "PC", "xPSR"};

static void stack_dump(unsigned int* stack_value)
{
    int i;
    printk("Reg dump:\n");
    for (i = 0; i < LAZY_STACKING_CONTEXT_SIZE; ++i)
    {
        if (i)
            printk(", ");
        printk("%s: %08X", __REG_NAMES[i], stack_value[i]);
    }
    printk("\nStack dump:\n");
    dump((unsigned int)(stack_value + LAZY_STACKING_CONTEXT_SIZE), 0x100);
}
#endif //KERNEL_DEBUG

static void process_fault(unsigned int ret_value)
{
    //from thread context, just killing thread
    if (ret_value == PSP_IN_LR)
    {
        kprocess_destroy(kprocess_get_current());
        SCB_CFSR = 0;
    }
    else
        panic();
}

void on_hard_fault(unsigned int ret_value, unsigned int* stack_value)
{
#if (KERNEL_DEBUG)
        printk("HARD FAULT: ");
        if (SCB_HFSR & HFSR_VECTTBL)
            printk("Vector table read fault\n");
        //wrong sys call
        else if (*(uint16_t*)(stack_value[PC_IN_STACK] - 2) == SVC_12)
            printk("SYS call while disabled interrupts\n");
        else
            printk("General hard fault at %#.08x\n");
        stack_dump(stack_value);
#endif
    if (ret_value == PSP_IN_LR && (*(uint16_t*)(stack_value[PC_IN_STACK] - 2) == SVC_12))
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
            printk("Stacking failed\n");
        else if (SCB_CFSR & CFSR_MUNSTKERR)
            printk("Unstacking failed\n");
        stack_dump(stack_value);
#endif
        panic();
    }
    else if (SCB_CFSR & (CFSR_DACCVIOL | CFSR_IACCVIOL))
    {
#if (KERNEL_DEBUG)
        printk("MEM MANAGE: ");
        if (SCB_CFSR & CFSR_DACCVIOL)
            printk("Data access violation\n");
        else if (SCB_CFSR & CFSR_IACCVIOL)
            printk("Instruction access violation\n");
        stack_dump(stack_value);
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
            printk("Stacking failed\n");
        else if (SCB_CFSR & CFSR_BUNSTKERR)
            printk("Unstacking failed\n");
        else if (SCB_CFSR & (CFSR_IMPRECISERR | CFSR_PRECISERR))
            printk("Data bus error at %#.08x\n", SCB_BFAR);
        else if (SCB_CFSR & CFSR_IBUSERR)
            printk("Instruction bus error at %#.08x\n", SCB_BFAR);
        stack_dump(stack_value);
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
        printk("Division by zero\n");
    else if (SCB_CFSR & CFSR_UNALIGNED)
        printk("Unaligned access\n");
    else if (SCB_CFSR & CFSR_NOCP)
        printk("No coprocessor found\n");
    else if (SCB_CFSR & (CFSR_UNDEFINSTR | CFSR_INVPC))
        printk("Undefined instruction\n");
    else if (SCB_CFSR & CFSR_INVSTATE)
        printk("Invalid state\n");
    stack_dump(stack_value);
#endif

    process_fault(ret_value);
}
