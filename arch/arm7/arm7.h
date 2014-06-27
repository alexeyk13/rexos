/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARM7_H
#define ARM7_H

#ifndef __ASSEMBLER__
#include "cc_macro.h"
#include "irq_arm7.h"
#endif //__ASSEMBLER__

#define SVC_MODE					0x13
#define IRQ_MODE					0x12
#define FIQ_MODE					0x11
#define ABORT_MODE				0x17
#define UNDEFINE_MODE			0x1b
#define SYS_MODE					0x1f
#define USER_MODE					0x10

#define I_BIT						0x80
#define F_BIT						0x40

#ifndef __ASSEMBLER__

__attribute__( ( always_inline ) ) __STATIC_INLINE void __NOP(void)
{
	__ASM volatile ("nop");
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __SWI(uint32_t num)
{
	__ASM volatile ("swi %0" : : "n" (num));
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_MODE(void)
{
	uint32_t result;
	__ASM volatile ("mrs %0, cpsr" : "=r" (result) );
	return(result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __set_MODE(uint32_t mode)
{
	__ASM volatile ("msr cpsr_c, %0" : : "r" (mode));
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_SP(void)
{
	uint32_t result;
	__ASM volatile ("mov %0, sp" : "=r" (result) );
	return(result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_IRQ_SP(void)
{
	uint32_t result;
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "msr cpsr_c, #0xd2\n\t"
						 "mov %0, sp\n\t"
						 "msr cpsr_c, r1": "=r" (result) :: "r1");
	return(result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_FIQ_SP(void)
{
	uint32_t result;
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "msr cpsr_c, #0xd1\n\t"
						 "mov %0, sp\n\t"
						 "msr cpsr_c, r1": "=r" (result) :: "r1");
	return(result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_ABORT_SP(void)
{
	uint32_t result;
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "msr cpsr_c, #0xd7\n\t"
						 "mov %0, sp\n\t"
						 "msr cpsr_c, r1": "=r" (result) :: "r1");
	return(result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_UNDEFINE_SP(void)
{
	uint32_t result;
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "msr cpsr_c, #0xdb\n\t"
						 "mov %0, sp\n\t"
						 "msr cpsr_c, r1": "=r" (result) :: "r1");
	return(result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __set_SP(uint32_t sp)
{
	__ASM volatile ("mov sp, %0" : : "r" (sp) );
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __disable_irq(void)
{
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "orr r1, r1, #0x80\n\t"
						 "msr cpsr_c, r1" : : : "r1", "cc");
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __disable_fiq(void)
{
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "orr r1, r1, #0x40\n\t"
						 "msr cpsr_c, r1" : : : "r1", "cc");
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __enable_irq(void)
{
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "bic r1, r1, #0x80\n\t"
						 "msr cpsr_c, r1" : : : "r1", "cc");
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __enable_fiq(void)
{
	__ASM volatile ("mrs r1, cpsr\n\t"
						 "bic r1, r1, #0x40\n\t"
						 "msr cpsr_c, r1" : : : "r1", "cc");
}
#endif //__ASSEMBLER__

#endif // ARM7_H
