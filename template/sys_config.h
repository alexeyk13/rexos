/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

/*
    config.h - userspace config
 */

//if set, after sys startup, __APP REx will be executed
#define SYS_APP                                             1
//add some debug info in SYS
#define SYS_INFO                                            1
//powersave mode.
#define SYS_POWERSAVE                                       0

//------------------------------ modules ---------------------------------------------
//enable/disable UART module
#define UART_MODULE                                         1

#endif // SYS_CONFIG_H
