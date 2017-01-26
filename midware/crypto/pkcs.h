/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef PKCS_H
#define PKCS_H

int eme_pkcs1_v1_15_decode(const void *em, unsigned int em_size, void* m, unsigned int m_max);
int pkcs7_decode(void* em_m, unsigned int size);
unsigned int pkcs7_encode(void* m, unsigned int size, unsigned int block_size);

#endif // PKCS_H
