/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "pkcs.h"
#include <stdint.h>
#include <string.h>

int eme_pkcs1_v1_15_decode(const void* em, unsigned int em_size, void* m, unsigned int m_max)
{
    const uint8_t* emc = em;
    int offset;
    //EM = 0x00 || 0x02 || PS || 0x00 || M
    if (em_size < 3)
        return -1;
    if ((emc[0] != 0x00) || (emc[1] != 0x02))
        return -1;
    for(offset = 2; (offset < em_size) && emc[offset]; ++offset) {}
    if (offset == em_size)
        return -1;
    ++offset;
    if (em_size - offset > m_max)
        return -2;
    memcpy(m, emc + offset, em_size - offset);
    return em_size - offset;
}
