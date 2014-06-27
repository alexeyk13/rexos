About											{#about}
=====

RExOS is embedded RTOS, based on exokernel architecture.
It's designed for ARM microprocessors, however, can be easily ported to any
32bit MCU.

Warning! In early development stage. Not ready for production.

Features
========

- LGPL License, open source.
- Tickless. For system scheduling used 1 HPET timer and RTC. If RTC is
unavailable, another HPET timer can be used for RTC emulation (with full RTC
functionality)
- Hardware abstraction.RTOS is providing common interface for drivers.
- Standart microkernel syncronization: mutexes, events, semaphores, queues.
- Nested mutex priority inheritance
- Embedded dynamic memory manager, with ability of nested memory pools for 
  threads, dedicated system pool. Aligned malloc calls inside of any pools
- Safe and MPU ready. All supervisor-specific calls are wrapped around 
  swi/svc calls for context rising.
- Embedded libraries:
  * printf/sprintf. Around 1k of code.
  * time routines. POSIX-light
  * single-linked list, double linked list, ring buffer, block ring buffer
  * random number generation: hardware (if supported) or software
- Error handling:
  * kill thread on system error, print error, if configured
  * restart system on critical error, memory dump if configured
  * handle error exceptions, decode address (if supported) and exception code
- Lot of debug features:
  * memory dumps
  * object marks
  * thread profiling: name, uptime, stack allocated/current/used
  * memory profiling: red-markings, pool free/allocated size, objects fragmentation
  * stack profiling: supervisor allocated/current/used. Plus for arm7 - irq, fiq, abt
  * configurable debug console
- Supported hardware:
  * ARM7
  * cortex-m3, drivers for:STM32 F2 (gpio, uart, rcc, timer, dma, rand)

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
0.0.1
! Fork from M-Kernel (https://github.com/alexeyk13/mkernel)
! New dynamic memory pool manager
+ Process has own heap with own error handling, stack and heap, all inside paged block
- support for ARM7 broken
- support for STM32F2 broken
- support for softNVIC(ARM7) broken