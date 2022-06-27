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
#if defined(IMXRT)
        ITCM(rwx)  : ORIGIN = 0x00000020, LENGTH = 128K
        DTCM(rw)  : ORIGIN = 0x20000000, LENGTH = 128K
        OCRAM(rw)  : ORIGIN = 0x20200000, LENGTH = 512K
#endif
}

SECTIONS
{
#if defined(IMXRT)
    .boot_hdr : ALIGN(4)
    {
        FILL(0xff)
        KEEP(*(.boot_hdr.conf))
        . = 0x1000 ;
        KEEP(*(.boot_hdr.ivt))
        . = 0x1020 ;
        KEEP(*(.boot_hdr.boot_data))
        . = 0x1030 ;
        KEEP(*(.boot_hdr.dcd_data))
        . = 0x2000 ;
        __vectors_start__ = ABSOLUTE(.) ;
    } >FLASH

/DISCARD/ : {
    *(.ARM.exidx*)
    *(.init)
    *(.fini)
    *(.debug*)
  }

#endif 
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

#if defined(IMXRT)
    .data_RAM3 : ALIGN(4)
    {
        __load_data_RAM3 = LOADADDR(.data_RAM3);
        PROVIDE(__start_data_RAM3 = .) ;
        KEEP(*(.ram_func))
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM3 = .) ;
     } > ITCM AT>FLASH
    __size_ram = __end_data_RAM3 - __start_data_RAM3;


    .data(NOLOAD) :
    {
        . = . + 0x0c;
        _startm = .;
        *(.global_vars);
        *(.bss);
        *(.data);
        _endm = .;
    } > RAM
#else
    _end = . ;
     PROVIDE(__END = .);
#endif // IMXRT
}

