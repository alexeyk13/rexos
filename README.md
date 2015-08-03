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
- Syncronization: ipc, stream, io
- Soft timers
- Embedded dynamic memory manager, for every process
- Safe and MPU ready. All supervisor-specific calls are wrapped around 
  swi/svc calls for context rising.
- Lib-level libraries:
  * ustdio: printf, sprintf. Around 2k of code.
  * ustdlib: malloc, free, realloc, atou, utoa
  * time routines
  * dynamic arrays
  * GPIO bitbang library
- User libraries:
  * USB routines library
  * GUI library: canvas, font, graphics, utf8 decoding
  * single-linked list, double linked list, ring buffer, 
  * rand
- Device stacks:
  * USB device stack with USB composite and vendor requests support
  * USB device CDC ACM class
  * USB device HID Boot Keyboard class
  * USB device CCID class
  * USB device MSC class
  * SCSI stack: SPC5, SBC3
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
- Supported architecture:
  * cortex-m0
  * cortex-m3
  * cortex-m4
  * ARM7
- Drivers:
  * core (PIN/GPIO, UART, TIMER, POWER): STM32F1, STM32F2, STM32F4, STM32L0, LPC11Uxx, LPC18xx
  * rtc: STM32F1, STM32F2, STM32F4, STM32L0
  * wdt: STM32F1, STM32F2, STM32F4, STM32L0
  * EEPROM: LPC11Uxx, STM32L0
  * I2C: LPC1Uxx
  * ADC: STM32F1, STM32L0
  * DAC: STM32F1, STM32L0
  * USB: STM32F1_CL, STM32L0, LPC11Uxx
  * ETH: STM32F1
  * МЭЛТ mt12864j LCD display

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
0.3.5
- LPC18xx USB0 support
- IPC size is now fixed by KERNEL_IPC_COUNT
- simplified process name store mechanism
- lib_heap removed
- REX_HEAP_FLAGS is now just REX_FLAG_PERSISTENT_NAME
- PROCESS -> KPROCESS, HEAP -> PROCESS
- __HEAP -> __PROCESS

0.3.4
- LPC18xx basic support: PIN/GPIO, UART, TIMER, POWER

0.3.3
- due to different meaning in LPC18xx of PIN and GPIO, HAL_GPIO is now HAL_PIN. GPIO is only userspace library, while PIN
  configuration is in driver space
- GPIO is now required library
- LPC18xx architecture decoding
- LPC18xx GPIO driver
- KERNEL_INFO now called KERNEL_DEBUG
- fix CCID EP size request halts

0.3.2
- Y2037 compatible, no POSIX time_t is used anymore. Up to 1M year date supported with BC (by design)
- TIME is now SYSTIME, time_t now TIME
- STM32 RTC driver update for new TIME interface
- SCSI stack: SBC3 write caching support

0.3.1
- USB mass storage class
- SCSI stack: SPC5, SBC3

0.3.0
- New sync object - io. block successor with embedded header and stack
- block and direct are now deprecated
- lpc/stm32 drivers, usb device stack moved to io interface
- array now userspace library
- ccid device interface simplified
- usb descriptor parser moved to userspace usb part
- usb descriptor registering interface simplified
- ipc_read now automatically answer on IPC_PING and clearing last error

0.2.9
- HAL is now part of cmd in IPC
- new HAL item: USBD_IFACE. USBD class interface updated
- removed obsolete mutex, event, semaphore from project code

0.2.8
- LPC UART, timer HAL interface
- STM32 L0 EEPROM driver

0.2.7
- Run time power management support with HAL interface
- hardware timer (htimer) HAL interface
- UART HAL interface
- STM32 HAL drivers: UART, power, timer
- STM32 L0 low power support
- USB device virtualbox workaround

0.2.6
- DAC driver HAL abstraction
- ADC driver HAL abstraction
- STM32 analog driver splitted into ADC and DAC
- STM32 L0 ADC and DAC drivers
- software wave generation module for driver
- IPC call waits for exact command, sended to process
- fix unsafe task switching, when process statistics is enabled

0.2.5
- STM32 F1 ehternet driver with double buffering supported
- TCP/IP scratch: MAC, MAC filtering, ARP resolve/announce, ARP cache, IP, ROUTE, ICMP echo, ICMP ping, ICMP flow control
- ARRAY refactoring: no more division operations, interface is more simple. Now it's required library.
- libusb splitted to midware USB descriptor helpers and userspace libusb
- soft rand routines now userspace lib
- GUI is now userspace lib

0.2.4
- LPC 11Uxx EEPROM driver
- no more splitted calls. Response is same as request
- direct read/write can work in partial mode
- new file API. IPC_IO_CANCELLED, IPC_READ_COMPLETE, IPC_WRITE_COMPLETE is now obsolete.
- file API supports direct mode
- new sys interface. Recommended abstraction: param1 - handle, param2 - object, param3 - size or error. 
  Get returns value in param2.
- critical fix in IPC queue
- USB CCID device state machine fix
- USB device support for vendor specific requests
- USB device suspend/resume support

0.2.3
- USB CCID device class
- USB device support wakeup/resume on linux
- LPC USB driver fixes
- IPC_IO_CANCEL, IPC_IO_CANCELLED file abstraction
- kprocess time statistics fixed

0.2.2
- kernel soft timers
- USB device HID Boot keyboard class
- fix fifo alignment in LPC USB driver

0.2.1
- GUI library
- MT driver update for GUI objects support
- LPC USB minor fixes

0.2.0
- drivers for LPC11Uxx USB device, МЭЛТ LCD display
- GPIO bitbang is now library for fast IO (lib_gpio), implementation is hw-specific
- USB helpers implemented as library (lib_usb)
- ARRAY (lib_array) supports custom data size
- USB classes now interface of USB device
- USB composite device support
- KERNEL_OBJECTS more standartized, requiring less parameter transfer
- most of headers are going to /userspace, less dependancy required
- critical fixes in memory pool, stream, ipc.
- new virtual handles: ANY_HANDLE, KERNEL_HANDLE

0.1.3
- drivers for LPC11Uxx
- STM32 bitbang driver template
- file api mode support

0.1.2
- dynamic BLOCK links
- some optimization in kernel_config.h
- HAL drivers for STM32
- STM32 analog, USB, UART can be optionally monolith for small systems

0.1.1
- global lib refactoring
- lib heap
- persistent names flag for process
- some STM32 driver refactoring
- HAL driver abstraction started

0.1.0
- No IDLE process required anymore. All idle execution is going on PendSV level
- Some critical bug fixes in process switching

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