/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
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
