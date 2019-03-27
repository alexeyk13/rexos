/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

/* exported global constant and functions */
	 .global svc_call

/* imported global constant and functions */

    .section    .text, "ax"
    .syntax unified
    .cpu cortex-m3
    .thumb

/*
	 extern unsigned int svc_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
 */

	 .thumb_func
svc_call:
	 svc		0x12
	 bx      lr
