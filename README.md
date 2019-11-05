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
- Embedded dynamic memory manager, for every process.
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
  * cryptography: AES, AES-CBC, SHA1, SHA256, HMAC-SHA1, HMAC-SHA256
  * crc
- Device stacks:
  * USB device stack with USB composite and vendor requests support
  * USB device CDC ACM class
  * USB device HID Boot Keyboard class
  * USB device CCID class
  * USB device MSC class with multiple LUN and DVD-ROM support
  * USB device RNDIS class
  * SCSI stack: SPC5, SBC3, MMC6
  * TCP/IP: MAC level 802.3, RFC768, RFC791, RFC792,  RFC793, RFC826
  * IP: fragmentation supported
  * ICMP: ECHO and flow control
  * ARP: timeouts and static routes support
  * DNS server
  * DHCP server
  * Web Server
  * TLS 1.2 Server (beta)
  * SD/MMC host stack
  * uCanOpen stack
  * FAT16 file system
  * SFS / SFS v2 - small file system (commercial license)
  * Block error rate layer with transactioning support
  * BER v2 (commercial license)
  * Log file systm
  * LoRa stack
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
- MCU Drivers:
  * ST STM32F0: generic, UART, RTC, WDT, SPI, I2C, USB, CAN
  * ST STM32F1: generic, UART, RTC, WDT, I2C, ADC, DAC, USB, ETH, flash
  * ST STM32F2: generic, UART, RTC, WDT, USB
  * ST STM32F4: generic, UART, RTC, WDT, USB
  * ST STM32L0: generic, UART, RTC, WDT, EEPROM, ADC, DAC, USB
  * ST STM32L1: generic, UART, RTC, WDT, I2C, USB
  * NXP LPC11Uxx: generic, UART, EEPROM, USB, flash
  * NXP LPC18xx: generic, UART, USB, ETH, SD/MMC, flash
  * NXP Kinetis K22: generic, UART, RTC, WDT, I2C, USB, flash (commercial license)
  * NXP Kinetis K32W: generic, UART, USB, flash, ADC, Radio BLE (alpha) (commercial license)
  * TI CC26x0: generic, UART, RTC, WDT, Radio BLE (alpha)
  * NRF51, NRF52: generic, UART, RTC, Radio BLE (alpha)
- Other drivers:
  * МЭЛТ mt12864j LCD display
  * ESP8266
  * RF: SX1261, SX1271, SX1272

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
0.6.4
- NRF51, NRF52 support
- Kinetis K32W support

0.6.3
- added Log File System (LFS) with flash retantion
- crc library

0.6.2
- LoRa device stack
- drivers for sx1261, 127x

0.6.1
- SFS is now non 2^n aligned (commercial license)
- BER v2 is up to 6 time more fast, than BER v1 (commercial license)
- MK22 flash driver rewritten for small blocks access (commercial license)

0.6.0
- ESP8266 driver
- fix multi-pool allocation error
- SVC calls trace for deep kernel debugging


0.5.9
- uart timeouts in US
- configurable comm timeouts
- CCID major refactoring, clock/rate support
- fix LPC otg memory leak
- fix MSCD multiple lun removable detection
- removed heap module
- kernel IO debug

0.5.8
- BER power-safe transaction support
- SFS - file system for internal flash (commercial license)
- CCID fix windows card presence
- STM32 ETH integrated in driver space

0.5.7
- STM32 drivers are now exodrivers. System is now fully exodrivers based
- STM32 F1 flash driver
- STM32 F1 I2C driver (master & slave)
- STM32 F0 SPI driver
- uCanOpen support PDO/SDO, persistent data saving
- CCID WTX fix (thanks to Roma Jam @github)


0.5.6
- TLS added TLS_RSA_WITH_AES_128_CBC_SHA256 cipher suite
- DNS server
- DHCP server
- STM32 F0 CAN driver
- uCanOpen (alpha)

0.5.5
- WEB server rewritten: 
    * payload no more limited to MTU
    * multiple sessions support
    * custom url/parameters/data processing available
    * sessions timeouts, keep-alive or close immediatly
- STM32L1 support (thanks to Roma Jam @github)
- LPC exodrivers (experimental)

0.5.4
- BER bad block recovery
- BER error statistics
- FAT16 get free/used space
- critical exodrivers IPC fix

0.5.3
- CC26x0 Radio BLE alpha support
- HEAP sync object for dynamic object sharing
- MK22 flash support (commercial license)

0.5.2
- TI CC26x0 support
- stream write critical fix

0.5.1 
- PIN hal abstraction
- fix critical kipc_post error on exodrivers
- USB sync method to avoid driver post overflow

0.5.0
- kernel-kernel IPC calls. Exodrivers support. Experimental feature
- isr own error processing
- non-block stream ISR/kernel access
- support for early hardware init
- support for cortex-mX custom flash layout
- no more generated asm_defines.h
- STM32 F102, F103 USB driver support. Thanks to zurabob

0.4.9
- LPC18xx flash storage interface
- Block error rate for filesystem over flash support
- FAT16 minor fixes

0.4.8
- Support of FAT16 file system: read, write, format
- FAT16 VFS drafts

0.4.7
- STM32F0 USB driver
- STM32F0 UART IO mode driver
- USB drivers now part of CORE drivers, no more dedicated SYS_OBJ_USB handle

0.4.6
- STM32 F0 support
- STM32 HAL interface for clocks query

0.4.5
- LPC18xx power profiling support
- HAL syntax for frequencies request
- Minor fixes in LPC18xx drivers

0.4.4
- USB Mass storage multiple LUN support
- MMC6 SCSI
- USB Mass storage DVD-ROM support
- GPIO now userspace library
- Dynamic arrays in kernel by lib layer (Array, SO)
- Multiple system pools support with dynamic configuration
- LPC OTG driver DTD conveyor to support IO's more than 20KB
- LPC18xx every memory chunk is transparent available for user
- Extended pool statistics in process_info()

0.4.3
- SD/MMC host interface
- HAL type independent storage interface
- SCSI refactoring for making storage interface compatible
- USB device configuration templates
- USB MSC transparent storage interface
- LPC18xx SD/MMC driver

0.4.2
- Some cryptography: AES, AES-CBC, SHA1, SHA256, HMAC-SHA1, HMAC-SHA256
- TLS 1.2 support (beta). Transparent to TCP/IP stack

0.4.1
- USB RNDIS class support
- USB interface associations descriptor support
- USB transparent HAL interface
- minor fixes in HTTP server, TCP/IP stack

0.4.0
- HTTP server (beta)
- LPC18xx ethernet driver
- minor TCP/IP fixes

0.3.9
- TCP/IP stack full support. Fragmentation safe
- SO: fragmentation-safe object handles support library
- Example updated to support both USB (cdc echo) and TCP/IP telnet(23) echo

0.3.8
- IPC request flag. No need to check if response on IPC is required anymore
- HAL_REQ, HAL_IO_REQ helpers to make CMD for IPC request
- IPC get refactoring:
  - get_handle when handle is expected. If error is set, INVALID_HANDLE is returned
  - get_size for IO operations. Negative value means error
  - get_int for generic int. Error is set, but original param2 returned
- error codes refactoring. Synthetic ERROR_SYNC added as helper for asynchronous IPC request completion
- IP stack refactoring
- ARP static routes and debug info
- ICMP echo request simplified. More detailed flow control

0.3.7
- UART flush HAL
- UART IO interface for block transfers
- LPC uart IO interface
- soft timers start/stop in ISR
- USB device high-speed fix

0.3.6
- LPC18xx USB1 & both usb support
- LPC18xx, LPC11U6x I2C support
- I2C HAL interface

0.3.5
- LPC18xx USB0 support
- IPC size is now fixed by KERNEL_IPC_COUNT
- simplified process name store mechanism
- lib_heap removed
- REX_HEAP_FLAGS is now just REX_FLAG_PERSISTENT_NAME
- PROCESS -> KPROCESS, HEAP -> PROCESS
- __HEAP -> __PROCESS
- size of created processes not includes process system header
- pinboard soft timer interface
- removed ipc/io calls timeouts requests. Use soft timers instead
- IO is now integrated part of IPC. HAL_IO_CMD added to work as helper
- SYS_OBJ_PINBOARD, SYS_OBJ_USBD objects not created by process, only hardware drivers and STD
- IPC queue in userspace
- IPC message peek in userspace
- EEP HAL interface updated: no more seek. handle is offset from base
- LPC, STM32 EEP HAL update

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
