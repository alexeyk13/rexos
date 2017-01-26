/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CONV_H
#define CONV_H

#include <stdint.h>

int hex_decode(char* text, uint8_t* data, unsigned int size_max);
void hex_encode(uint8_t* data, unsigned int size, char* text);

#endif // CONV_H
