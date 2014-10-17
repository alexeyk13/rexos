About											{#about}
=====

RExOS is embedded RTOS, based on exokernel architecture.
It's designed for ARM microprocessors, however, can be easily ported to any
32bit MCU.

Features
========

- LGPL License, open source.
- Tickless. For system scheduling used 1 HPET timer and RTC. If RTC is
unavailable, another HPET timer can be used for RTC emulation
- Very thin kernel. 
- Independent library level, accessible both from kernel and userspace
- Independent system and drivers in userspace
- Exokernel syncronization: ipc, stream, direct IO, block
- Standart microkernel syncronization: mutexes, events, semaphores. Nested mutex priority inheritance.
- Embedded dynamic memory manager, for every process
- Safe and MPU ready. All supervisor-specific calls are wrapped around 
  swi/svc calls for context rising.
- Embedded libraries:
  * ustdio: printf, sprintf. Around 2k of code.
  * ustdlib: time, rand.
  * single-linked list, double linked list, ring buffer, dynamic arrays
- Error handling:
  * each process has own error processing
  * kernel panic with memory dump on critical errors. Restart system if configured
  * handle hardware error exceptions, decode address and exception codes
- Lot of debug features:
  * memory dumps
  * object marks
  * sys objects range checking
  * user objects range checking
  * thread profiling: name, uptime, stack allocated/used
  * memory profiling: red-markings, pool free/allocated size, objects fragmentation
- Supported hardware:
  * ARM7
  * cortex-m3, drivers for:STM32 F1/F2/F4 - power, gpio, uart, timer, rtc(F1 only)

Cortex-M3 features:
------------------
- Native SVC and pendSV support

ARM7 features:
-------------
- Nested interrupts
- FIQ support
- pendSV emulation, when returning to user/system context

History
=======
0.0.5 
- No system process anymore, replaced by kernel objects
- IRQ handlers now in dynamic memory, saving some space
- persistent kernel name
- kernel/user stack profiling with max stack usage

0.0.4
- interrupt friendly kernel calls for faster realtime response
- dynamic array library
- (system) basic file level abstraction
- (system) USB STM32F1 CL drivers
- (system) USB device stack
- (system) USB CDC stack

0.0.3
- new sync objects: direct IO, block
- mutex, events, sempaphores are now deprecated

0.0.2
- new sync objects: ipc, stream
- new sys level - userspace system
- support for STM32F2 fixed (not tested)

0.0.1
- Fork from M-Kernel (https://github.com/alexeyk13/mkernel)
- New dynamic memory pool manager
- Process has own heap with own error handling, stack and heap, all inside paged block
- support for ARM7 broken
- support for STM32F2 broken
- support for softNVIC(ARM7) broken