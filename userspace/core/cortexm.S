/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

/* exported global constant and functions */
	 .global sys_call

/* imported global constant and functions */

    .section    .text, "ax"
    .syntax unified
    .cpu cortex-m3
    .thumb

/*
	 extern unsigned int sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
 */

	 .thumb_func
sys_call:
	 svc		0x12
	 bx      lr
