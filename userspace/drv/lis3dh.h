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
#define LIS_REG_STATUS_AUX                          0x07
#define LIS_REG_ADC1_L                              0x08
#define LIS_REG_ADC1_H                              0x09
#define LIS_REG_ADC2_L                              0x0a
#define LIS_REG_ADC2_H                              0x0b
#define LIS_REG_ADC3_L                              0x0c
#define LIS_REG_ADC3_H                              0x0d
#define LIS_REG_WHO_AM_I                            0x0f
#define LIS_REG_CTRL0                               0x1e
#define LIS_REG_TEMP_CFG                            0x1f
#define LIS_REG_CTRL1                               0x20
#define LIS_REG_CTRL2                               0x21
#define LIS_REG_CTRL3                               0x22
#define LIS_REG_CTRL4                               0x23
#define LIS_REG_CTRL5                               0x24
#define LIS_REG_CTRL6                               0x25
#define LIS_REG_REFERENCE                           0x26
#define LIS_REG_STATUS                              0x27
#define LIS_REG_OUT_X_L                             0x28
#define LIS_REG_OUT_X_H                             0x29
#define LIS_REG_OUT_Y_L                             0x2a
#define LIS_REG_OUT_Y_H                             0x2b
#define LIS_REG_OUT_Z_L                             0x2c
#define LIS_REG_OUT_Z_H                             0x2d
#define LIS_REG_FIFO_CTRL                           0x2e
#define LIS_REG_FIFO_SRC                            0x2f
#define LIS_REG_INT1_CFG                            0x30
#define LIS_REG_INT1_SRC                            0x31
#define LIS_REG_INT1_THS                            0x32
#define LIS_REG_INT1_DURATION                       0x33
#define LIS_REG_INT2_CFG                            0x34
#define LIS_REG_INT2_SRC                            0x35
#define LIS_REG_INT2_THS                            0x36
#define LIS_REG_INT2_DURATION                       0x37
#define LIS_REG_CLICK_CFG                           0x38
#define LIS_REG_CLICK_SRC                           0x39
#define LIS_REG_CLICK_THS                           0x3a
#define LIS_REG_TIME_LIMIT                          0x3b
#define LIS_REG_TIME_LATENCY                        0x3c
#define LIS_REG_TIME_WINDOW                         0x3d
#define LIS_REG_ACT_THS                             0x3e
#define LIS_REG_ACT_DUR                             0x3f

#define LIS_REG_AUTOINCREMENT                       (1 << 7)

#define LIS_REG_CTRL1_XEN                           (1 << 0)
#define LIS_REG_CTRL1_YEN                           (1 << 1)
#define LIS_REG_CTRL1_ZEN                           (1 << 2)
#define LIS_REG_CTRL1_LPEN                          (1 << 3)
#define LIS_REG_CTRL1_ODR_POWER_DOWN                (0 << 4)
#define LIS_REG_CTRL1_ODR_1HZ                       (1 << 4)
#define LIS_REG_CTRL1_ODR_10HZ                      (2 << 4)
#define LIS_REG_CTRL1_ODR_25HZ                      (3 << 4)
#define LIS_REG_CTRL1_ODR_50HZ                      (4 << 4)
#define LIS_REG_CTRL1_ODR_100HZ                     (5 << 4)
#define LIS_REG_CTRL1_ODR_200HZ                     (6 << 4)
#define LIS_REG_CTRL1_ODR_400HZ                     (7 << 4)
#define LIS_REG_CTRL1_ODR_LP_1600HZ                 (8 << 4)
#define LIS_REG_CTRL1_ODR_1344HZ                    (9 << 4)
#define LIS_REG_CTRL1_ODR_LP_5376HZ                 (9 << 4)

#define LIS_REG_CTRL3_HP_IA1                        (1 << 0)
#define LIS_REG_CTRL3_HP_IA2                        (1 << 1)
#define LIS_REG_CTRL3_HP_CLICK                      (1 << 2)
#define LIS_REG_CTRL3_FDS                           (1 << 3)
#define LIS_REG_CTRL3_HPCF                          (3 << 4)
#define LIS_REG_CTRL3_HPM_NORMAL_RESET              (0 << 6)
#define LIS_REG_CTRL3_HPM_REFERENCE                 (1 << 6)
#define LIS_REG_CTRL3_HPM_NORMAL                    (2 << 6)
#define LIS_REG_CTRL3_HPM_AUTORESET                 (3 << 6)


#define LIS_REG_CTRL4_SIM                           (1 << 0)
#define LIS_REG_CTRL4_ST_NORMAL                     (0 << 1)
#define LIS_REG_CTRL4_ST_TEST1                      (1 << 1)
#define LIS_REG_CTRL4_ST_TEST2                      (2 << 1)
#define LIS_REG_CTRL4_HR                            (1 << 3)
#define LIS_REG_CTRL4_FS_2G                         (0 << 4)
#define LIS_REG_CTRL4_FS_4G                         (1 << 4)
#define LIS_REG_CTRL4_FS_8G                         (2 << 4)
#define LIS_REG_CTRL4_FS_16G                        (3 << 4)
#define LIS_REG_CTRL4_BLE                           (1 << 6)
#define LIS_REG_CTRL4_BDU                           (1 << 7)

#define LIS3DH_WHO_AM_I_VALUE                       0x33

typedef struct _LIS3DH LIS3DH;

typedef enum {
    LIS_LOW_POWER = 0,
    LIS_NORMAL,
    LIS_HIGH_RESOLUTION
} LIS_POWER_MODE;

typedef struct {
    int16_t x, y, z;
} LIS_DATA;

LIS3DH* lis3dh_open(unsigned int i2c, uint8_t sla, LIS_POWER_MODE power_mode, unsigned int rate_hz);
void lis3dh_get(LIS3DH* lis3dh, LIS_DATA* lis_data);
//void lis3dh_close();


#endif // LIS3DH_H
