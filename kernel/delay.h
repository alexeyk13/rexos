/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DELAY_H
#define DELAY_H

/*
    common process delay
  */


extern void delay_us(unsigned int us);
//it's preferred to call sleep_ms
extern void delay_ms(unsigned int ms);

#endif // DELAY_H
