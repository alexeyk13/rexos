
MEMORY
{
 FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 0x10000000
 RAM (rw) : ORIGIN = 0x20000000, LENGTH = 0x10000000
}

SECTIONS
{
 .text :
 {
  KEEP(*(.reset));
  *(.text .text.* .gnu.linkonce.t.*)
  *(.glue_7t) *(.glue_7)
  *(.rodata .rodata.* .gnu.linkonce.r.*)
  . = ALIGN(4);
  _etext = .;
  PROVIDE(__ETEXT = .);
 } > FLASH

 _end = . ;
 PROVIDE(__END = .);
}
