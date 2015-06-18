/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef EEP_H
#define EEP_H

#include <stdbool.h>

/*
        EEPROM HAL interface
 */

bool eep_seek(unsigned int addr);
int eep_read(void* buf, unsigned int size);
int eep_write(const void* buf, unsigned int size);

#endif // EEP_H
