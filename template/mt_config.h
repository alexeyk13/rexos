/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MT_CONFIG_H
#define MT_CONFIG_H

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

#endif // MT_CONFIG_H
