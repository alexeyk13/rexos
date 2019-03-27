/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef WAVEGEN_H
#define WAVEGEN_H

#include "../userspace/dac.h"

void wave_gen(SAMPLE* buf, unsigned int cnt, DAC_WAVE_TYPE wave_type, SAMPLE amplitude);

#endif // WAVEGEN_H
