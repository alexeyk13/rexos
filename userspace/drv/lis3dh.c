/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lis3dh.h"
#include "../io.h"
#include "../stdlib.h"
#include "../i2c.h"
//remove me
#include "../stdio.h"

#define LIS3DH_IO_SIZE                          8

typedef struct _LIS3DH {
    unsigned int i2c;
    IO* io;
    uint8_t sla;
} LIS3DH;

static void lis3dh_destroy(LIS3DH* lis3dh)
{
    io_destroy(lis3dh->io);
    free(lis3dh);
}

LIS3DH* lis3dh_open(unsigned int i2c, uint8_t sla)
{
    LIS3DH* lis3dh = malloc(sizeof(LIS3DH));
    int res;
    if (lis3dh == NULL)
        return NULL;
    lis3dh->i2c = i2c;
    lis3dh->sla = sla;
    lis3dh->io = io_create(LIS3DH_IO_SIZE);
    if (lis3dh->io == NULL)
    {
        lis3dh_destroy(lis3dh);
        return NULL;
    }
    printf("tp0\n");
    if (i2c_read_addr(lis3dh->i2c, lis3dh->sla, LIS3DH_REG_WHO_AM_I, lis3dh->io, 1) < 1)
    {
        printf("res: %d, data: %x\n", res, *((char*)io_data(lis3dh->io)));
    }
    return lis3dh;
}
