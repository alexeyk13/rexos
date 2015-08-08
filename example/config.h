/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CONFIG_H
#define CONFIG_H

#define USB_PORT_NUM                                USB_0
#define USBD_PROCESS_SIZE                           900
#define USBD_PROCESS_PRIORITY                       150

#define DBG_CONSOLE                                 UART_1
#define DBG_CONSOLE_BAUD                            115200
#define DBG_CONSOLE_TX_PIN                          A9
#define DBG_CONSOLE_TX_PIN_AF                       AF4

#define TEST_ROUNDS                                 10000

#endif // CONFIG_H
