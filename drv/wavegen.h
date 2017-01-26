/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef WAVEGEN_H
#define WAVEGEN_H

#include "../userspace/dac.h"

void wave_gen(SAMPLE* buf, unsigned int cnt, DAC_WAVE_TYPE wave_type, SAMPLE amplitude);

#endif // WAVEGEN_H
