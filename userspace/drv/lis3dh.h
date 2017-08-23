/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIS3DH_H
#define LIS3DH_H

#include <stdbool.h>
#include <stdint.h>

//SLA depends on CS bit
#define LIS3DH_SLA_CS_LO                            0x18
#define LIS3DH_SLA_CS_HI                            0x19

//registers list
#define LIS3DH_REG_STATUS_AUX                       0x07
#define LIS3DH_REG_ADC1_L                           0x08
#define LIS3DH_REG_ADC1_H                           0x09
#define LIS3DH_REG_ADC2_L                           0x0a
#define LIS3DH_REG_ADC2_H                           0x0b
#define LIS3DH_REG_ADC3_L                           0x0c
#define LIS3DH_REG_ADC3_H                           0x0d
#define LIS3DH_REG_WHO_AM_I                         0x0f
#define LIS3DH_REG_CTRL0                            0x1e
#define LIS3DH_REG_TEMP_CFG                         0x1f
#define LIS3DH_REG_CTRL1                            0x20
#define LIS3DH_REG_CTRL2                            0x21
#define LIS3DH_REG_CTRL3                            0x22
#define LIS3DH_REG_CTRL4                            0x23
#define LIS3DH_REG_CTRL5                            0x24
#define LIS3DH_REG_CTRL6                            0x25
#define LIS3DH_REG_REFERENCE                        0x26
#define LIS3DH_REG_STATUS                           0x27
#define LIS3DH_REG_OUT_X_L                          0x28
#define LIS3DH_REG_OUT_X_H                          0x29
#define LIS3DH_REG_OUT_Y_L                          0x2a
#define LIS3DH_REG_OUT_Y_H                          0x2b
#define LIS3DH_REG_OUT_Z_L                          0x2c
#define LIS3DH_REG_OUT_Z_H                          0x2d
#define LIS3DH_REG_FIFO_CTRL                        0x2e
#define LIS3DH_REG_FIFO_SRC                         0x2f
#define LIS3DH_REG_INT1_CFG                         0x30
#define LIS3DH_REG_INT1_SRC                         0x31
#define LIS3DH_REG_INT1_THS                         0x32
#define LIS3DH_REG_INT1_DURATION                    0x33
#define LIS3DH_REG_INT2_CFG                         0x34
#define LIS3DH_REG_INT2_SRC                         0x35
#define LIS3DH_REG_INT2_THS                         0x36
#define LIS3DH_REG_INT2_DURATION                    0x37
#define LIS3DH_REG_CLICK_CFG                        0x38
#define LIS3DH_REG_CLICK_SRC                        0x39
#define LIS3DH_REG_CLICK_THS                        0x3a
#define LIS3DH_REG_TIME_LIMIT                       0x3b
#define LIS3DH_REG_TIME_LATENCY                     0x3c
#define LIS3DH_REG_TIME_WINDOW                      0x3d
#define LIS3DH_REG_ACT_THS                          0x3e
#define LIS3DH_REG_ACT_DUR                          0x3f

typedef struct _LIS3DH LIS3DH;

LIS3DH* lis3dh_open(unsigned int i2c, uint8_t sla);
//void lis3dh_close();


#endif // LIS3DH_H
