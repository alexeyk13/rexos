#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

//----------------------------------- kernel ------------------------------------------------------------------
//debug specific:
#define KERNEL_DEBUG								1
//marks objects with magic in headers
#define KERNEL_MARKS								1
//check range of dynamic objects and stacks. Only for kernel debug reasons
#define KERNEL_RANGE_CHECKING					1
//check context of system calls. Only for kernel debug reasons
#define KERNEL_CHECK_CONTEXT					1
//some kernel statistics
#define KERNEL_PROFILING						1
//halt system instead of reset
#define KERNEL_HALT_ON_FATAL_ERROR			1

//sys_timer specific:
#define SYS_TIMER_RTC							RTC_0
#define SYS_TIMER_HPET							TIM_4
#define SYS_TIMER_PRIORITY						10
#define SYS_TIMER_CACHE_SIZE					16
#define SYS_TIMER_SOFT_RTC						1
#define SYS_TIMER_SOFT_RTC_TIMER				TIM_7

//thread specific
#define THREAD_CACHE_SIZE						16
#define THREAD_IDLE_STACK_SIZE				32

#define THREAD_STACK_SIZE						0x00002000
#define SVC_STACK_SIZE							256
#define SYSTEM_POOL_SIZE						(3 * 1024)
//--------------------------------- clock --------------------------------------------------------------------
//max core freq by default
#define STARTUP_CORE_FREQ						0

//------------------------- modules configure --------------------------------------------------------------
//think twice, before disabling
#define DBG_CONSOLE								1
#define WATCHDOG_MODULE							0

//------------------------------------ console --------------------------------------------------------------
#define DBG_CONSOLE_BAUD						115200
#define DBG_CONSOLE_UART						UART_1
//!13
#define DBG_CONSOLE_IRQ_PRIORITY				2
#define DBG_CONSOLE_TX_FIFO					32
//!!
#define DBG_CONSOLE_THREAD_PRIORITY			5
#define DBG_CONSOLE_THREAD_STACK_SIZE		64
//------------------------------- random numbers ------------------------------------------------------------
#define RANDOM_DEBUG								1
//------------------------------------- I2C -----------------------------------------------------------------
#define I2C_DEBUG									1
//use with care. May halt transfers
#define I2C_DEBUG_TRANSFERS					0

//------------------------------------- USB -----------------------------------------------------------------
//change in case you have long string descriptors
#define USBD_CONTROL_BUF_SIZE					64

#define USB_DEBUG_SUCCESS_REQUESTS			0
#define USB_DEBUG_ERRORS						1
//only for hw driver debug
#define USB_DEBUG_FLOW							0
//#define USB_IRQ_PRIORITY						10
#define USB_IRQ_PRIORITY						0xff
//------------------------------------- USB MSC ------------------------------------------------------------
#define USB_MSC_THREAD_PRIORITY				10
#define USB_MSC_THREAD_STACK_SIZE			128

//must be at least 2 for double-buffering
#define USB_MSC_BUFFERS_IN_QUEUE				2
//more sectors in block, faster transfers
#define USB_MSC_SECTORS_IN_BLOCK				50

//only for hw driver debug
#define USB_MSC_DEBUG_FLOW						0
//---------------------------------- SCSI ------------------------------------------------------------------
#define SCSI_DEBUG_ERRORS						1
//debug all command completion
#define SCSI_DEBUG_FLOW							0
#define SCSI_DEBUG_IO_FAIL						1
#define SCSI_DEBUG_UNSUPPORTED				0
//----------------------------------- SD_CARD ------------------------------------------------------------------
#define SD_CARD_CD_PIN_ENABLED				1
#define SD_CARD_CD_PIN							GPIO_A12
#define SD_CARD_DEBUG							0
#define SD_CARD_DEBUG_FLOW						0
#define SD_CARD_DEBUG_ERRORS					0

#define SDIO_IRQ_PRIORITY						6

#define STORAGE_RETRY_COUNT					3
//----------------------------------- keyboard ----------------------------------------------------------------
#define KEYBOARD_DEBOUNCE_MS					10
#define KEYBOARD_POLL_MS						100
#define KEYBOARD_THREAD_PRIORITY				10

#endif // KERNEL_CONFIG_H
