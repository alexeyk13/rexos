Getting started							{#gettingstarted}
===============

All following info is obsolete for RExOS. Will be updated little bit later...

Preparing project
=================

First of all, you need to copy shipped configuration files from template folder to your project working directory:

kernel_config.h
---------------

Is a main configuration file. Everything about hardware and kernel is configured here. It's recommended to keep here only
kernel-specific configuration. Store project-specific configuration in own file - config.h

kernel_config.h is highly sensitive for debug and profiling setting - more debug options you leave, more space compiled
project will require.

first of all, make sure, following options are configured right:

#define SYS_TIMER_HPET							TIM_4\n\n

#define SYS_TIMER_SOFT_RTC						0\n
#define SYS_TIMER_RTC							RTC_0\n\n

or:

#define SYS_TIMER_SOFT_RTC						1\n
#define SYS_TIMER_SOFT_RTC_TIMER				TIM_7\n\n


#define DBG_CONSOLE_BAUD						115200\n
#define DBG_CONSOLE_UART						UART_1\n

Makefile
--------

This file contains compilation rules for project and also defines global constants and target hardware. 

Critical options are:

\b TARGET_NAME				- name of your project\n
\b KERNEL					- relative (recommended) or absolute path to RExOS files\n

\b MCU_NAME 				- name of target MCU. Supported MCU you can find inside /arch folder. Or you can add your own custom MCU. Refer to porting help.\n

cortex-m3 specific:\n

\b CMSIS_DIR				- path to CMSIS library\n
\b CMSIS_DEVICE_DIR		- uncomment one of CMSIS device folder. For now, only STM32 F2xx is supported\n

Hardware config
---------------

It is hardware-dependent part of configuration. File name starts with hw_config_<your architecture>.h. For getting started it's enough to have this file in project

application_init
----------------

called after initial hardware is initialized and before interrupts are enabled. Initialize user hardware here (like gpio), initialize variables and create some tasks here. Look at \ref main.c in example
for more details.


That's all!

Everything rest will be done by RExOS, like:
 - preparing stacks
 - preparing memory manager
 - setting up pll, core frequency
 - starting debug console
 - ... and lots more
