/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

/*
    universal LD-script for ARM processors
 */
//if you've got error on this line, than this file is not preprocessed

#include "../userspace/core/core.h"

MEMORY
{
	FLASH (rx) : ORIGIN = FLASH_BASE, LENGTH =	0x10000000
	RAM (rw)   : ORIGIN = SRAM_BASE, LENGTH = 0x10000000
}

SECTIONS
{
	.text :
	{
                KEEP(*(.isr_vectors));
                KEEP(*(.flashconfig));
                KEEP(*(.reset));							/* Startup code */
                *(.text .text.* .gnu.linkonce.t.*)
		*(.glue_7t) *(.glue_7)
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		. = ALIGN(4);
		_etext = .;
		PROVIDE(__ETEXT = .);
	} > FLASH

    .init :
    {
        *(.init*);
    } >FLASH
    .fini :
    {
      *(.fini*);
    } >FLASH
    .heap :
    {
      *(.heap*);
    } >FLASH
    _end = . ;
	PROVIDE(__END = .);
}

