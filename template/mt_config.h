/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT_CONFIG_H
#define MT_CONFIG_H

#include "../../../rexos/userspace/stm32_driver.h"

//----------------------- pin definition -------------------------------------------
//port for data lines D0..D7
#define DATA_PORT                       GPIO_PORT_A
//pin numbers mask for data lines. Don't change.
#define DATA_MASK                       0xff

//port for address, chip select and direction lines
#define ADDSET_PORT                     GPIO_PORT_B
//chip select pin number
#define MT_CS1                          (1 << 7)
#define MT_CS2                          (1 << 6)
//read/write pin number
#define MT_RW                           (1 << 4)
//data/command pin number
#define MT_A                            (1 << 3)

//backlight pin
#define MT_BACKLIGHT                    B9
//reset pin
#define MT_RESET                        B5
//strobe pin
#define MT_STROBE                       A15

//------------------------------ timeouts ------------------------------------------
//all CLKS time. Refer to datasheet for more details
//Address hold time, min 140ns
#define TAS                             1
//Data read prepare time, max 320ns
#define TDDR                            2
//E high pulse time, min 450ns
#define PW                              3
//Delay between commands, min 8us
#define TW                              40
//Reset time, max 10us
#define TR                              50
//Reset impulse time, min 200ns
#define TRI                             2
//------------------------------ general ---------------------------------------------
//pixel test api
#define MT_TEST                         1

#define X_MIRROR                        1

#endif // MT_CONFIG_H
