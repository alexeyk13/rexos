/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lis3dh.h"
#include "../io.h"
#include "../stdlib.h"
#include "../i2c.h"
#include "../error.h"
#include <string.h>

#define LIS_IO_SIZE                                         6

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

static void lis3dh_set_reg(LIS3DH* lis3dh, uint8_t reg, uint8_t value)
{
    *((char*)io_data(lis3dh->io)) = value;
    lis3dh->io->data_size = 1;
    i2c_write_addr(lis3dh->i2c, lis3dh->sla, reg, lis3dh->io);
}

static uint8_t lis3dh_get_reg(LIS3DH* lis3dh, uint8_t reg)
{
    if (i2c_read_addr(lis3dh->i2c, lis3dh->sla, reg, lis3dh->io, 1) < 1)
        return 0xff;
    return *((char*)io_data(lis3dh->io));
}

LIS3DH* lis3dh_open(unsigned int i2c, uint8_t sla, LIS_POWER_MODE power_mode, unsigned int rate_hz)
{
    uint8_t reg;
    LIS3DH* lis3dh = malloc(sizeof(LIS3DH));
    if (lis3dh == NULL)
        return NULL;
    do {
        lis3dh->i2c = i2c;
        lis3dh->sla = sla;
        lis3dh->io = io_create(LIS_IO_SIZE + sizeof(I2C_STACK));
        if (lis3dh->io == NULL)
            break;

        //check version and device present
        if (lis3dh_get_reg(lis3dh, LIS_REG_WHO_AM_I) != LIS3DH_WHO_AM_I_VALUE)
            break;

        lis3dh_set_reg(lis3dh, LIS_REG_CTRL2, LIS_REG_CTRL3_FDS);

        //exit from power down
        reg = 0;
        switch (rate_hz)
        {
        case 1:
            reg = LIS_REG_CTRL1_ODR_1HZ;
            break;
        case 10:
            reg = LIS_REG_CTRL1_ODR_10HZ;
            break;
        case 25:
            reg = LIS_REG_CTRL1_ODR_25HZ;
            break;
        case 50:
            reg = LIS_REG_CTRL1_ODR_50HZ;
            break;
        case 100:
            reg = LIS_REG_CTRL1_ODR_100HZ;
            break;
        case 200:
            reg = LIS_REG_CTRL1_ODR_200HZ;
            break;
        case 400:
            reg = LIS_REG_CTRL1_ODR_400HZ;
            break;
        case 1600:
            if (power_mode == LIS_LOW_POWER)
                reg = LIS_REG_CTRL1_ODR_LP_1600HZ;
            break;
        case 1344:
            if (power_mode != LIS_LOW_POWER)
            reg = LIS_REG_CTRL1_ODR_1344HZ;
            break;
        case 5376:
            if (power_mode == LIS_LOW_POWER)
                reg = LIS_REG_CTRL1_ODR_LP_5376HZ;
        default:
            break;
        }
        if (reg == 0)
        {
            error(ERROR_INVALID_PARAMS);
            break;
        }
        reg |= LIS_REG_CTRL1_ZEN | LIS_REG_CTRL1_YEN | LIS_REG_CTRL1_XEN;
        if (power_mode == LIS_LOW_POWER)
            reg |= LIS_REG_CTRL1_LPEN;
        lis3dh_set_reg(lis3dh, LIS_REG_CTRL1, reg);

        reg = 0;
        if (power_mode == LIS_HIGH_RESOLUTION)
            reg |= LIS_REG_CTRL4_HR;
        lis3dh_set_reg(lis3dh, LIS_REG_CTRL4, reg);

        return lis3dh;
    } while (false);
    lis3dh_destroy(lis3dh);
    return NULL;
}


void lis3dh_get(LIS3DH* lis3dh, LIS_DATA* lis_data)
{
    if (i2c_read_addr(lis3dh->i2c, lis3dh->sla, LIS_REG_OUT_X_L | LIS_REG_AUTOINCREMENT, lis3dh->io, 6) == 6)
        memcpy(lis_data, io_data(lis3dh->io), 6);
}
