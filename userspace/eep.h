/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef EEP_H
#define EEP_H

#include <stdbool.h>

/*
        EEPROM HAL interface
 */

int eep_read(unsigned int offset, void* buf, unsigned int size);
int eep_write(unsigned int offset, const void* buf, unsigned int size);

#endif // EEP_H
