Architecture porting guide

Depends on core (now supported on ARM7 and cortex-m3):

functions, that must be defined:
int disable_interrupts(); //disable interrupts and return prior interrupts state
void restore_interrupts(int state); //restore interrupts to prior state
CONTEXT get_context(); //get current context
unsigned int do_sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3); //call to kernel
void pend_switch_context() //pend switching on supervisor exit
void thread_setup_context(THREAD* thread, THREAD_FUNCTION fn, void* param); //setup thread context on stack
void thread_patch_context(THREAD* thread, unsigned int res); //patch thread return value

defines, that must be provided:

SYS_RAM_BASE //	base address of system RAM. Provided for Cortex-M because of arch, but must be set for any other core.

callbacks provided:

unsigned int sys_handler(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3); // sys_call implementation
void startup(); //call from startup script before enabling interrupts
void abnormal_exit(); //abnormal thread exit

global variables provided:

_active_thread //thread, that is running now
_next_thread //thread to switch to. If NULL, no switch is required