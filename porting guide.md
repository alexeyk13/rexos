Architecture porting guide

Depends on core (now supported on ARM7 and cortex-m3):

functions, that must be defined:
int disable_interrupts(); //disable interrupts and return prior interrupts state
void restore_interrupts(int state); //restore interrupts to prior state
void reset(); // reset core. Infinite loop if not supported
unsigned int sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3); //call to kernel
void pend_switch_context() //pend switching on supervisor exit
void process_setup_context(PROCESS* process, PROCESS_FUNCTION fn); //setup thread context on stack

defines, that must be provided for mcu, if not decoded

SRAM_BASE // base address of system RAM. Provided for Cortex-M because of arch, but must be set for any other core.
SRAM_SIZE // size of system RAM. Provided for STM32(F1, F2, F4)
FLASH_BASE //base address of flash area. Provided for STM32


callbacks provided:

REX __INIT // userspace init thread.

global variables provided: