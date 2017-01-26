/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "wavegen.h"

#if (WAVEGEN_SINE)
static inline void wave_gen_sine(SAMPLE* buf, unsigned int cnt, SAMPLE amplitude)
{
#error Not implemented
}
#endif //WAVEGEN_SINE

#if (WAVEGEN_TRIANGLE)
static inline void wave_gen_triangle(SAMPLE* buf, unsigned int cnt, SAMPLE amplitude)
{
    int i;
    for (i = 0; i < (cnt >> 1); ++i)
        buf[i] = buf[cnt - i - 1] = (((amplitude * i) << 2) + 1) >> 1;
    buf[cnt << 1] = amplitude;
}
#endif //WAVEGEN_TRIANGLE

#if (WAVEGEN_SQUARE)
static inline void wave_gen_square(SAMPLE* buf, unsigned int cnt, SAMPLE amplitude)
{
    int i;
    for (i = 0; i < (cnt >> 1); ++i)
    {
        buf[i] = 0;
        buf[cnt - i - 1] = amplitude;
    }
    buf[cnt << 1] = amplitude;
}
#endif //WAVEGEN_SQUARE

void wave_gen(SAMPLE* buf, unsigned int cnt, DAC_WAVE_TYPE wave_type, SAMPLE amplitude)
{
    switch (wave_type)
    {
#if (WAVEGEN_SINE)
    case DAC_WAVE_SINE:
        wave_gen_sine(buf, cnt, amplitude);
        break;
#endif //WAVEGEN_SINE
#if (WAVEGEN_TRIANGLE)
    case DAC_WAVE_TRIANGLE:
        wave_gen_triangle(buf, cnt, amplitude);
        break;
#endif //WAVEGEN_TRIANGLE
#if (WAVEGEN_SQUARE)
    case DAC_WAVE_SQUARE:
        wave_gen_square(buf, cnt, amplitude);
        break;
#endif //WAVEGEN_SQUARE
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
