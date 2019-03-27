/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
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
