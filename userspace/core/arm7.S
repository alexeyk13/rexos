/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

/* exported global constant and functions */
	.global sys_call

/* imported global constant and functions */

/* data segment */

    .section    .text, "ax"
    .code 32

/*
	extern unsigned int sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
 */

sys_call:
	swi	0
	bx		lr
