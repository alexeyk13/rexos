/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "io.h"
typedef enum {
    SPI_MSBFIRST = 0,
    SPI_LSBFIRST
} SPI_ORDER;

typedef enum {
    SPI_DIV_2 = 0,
    SPI_DIV_4,
    SPI_DIV_8,
    SPI_DIV_16,
    SPI_DIV_32,
    SPI_DIV_64,
    SPI_DIV_128,
    SPI_DIV_256
} SPI_SPEED;

typedef struct {
    uint32_t cs_pin;
    uint8_t cpha;
    uint8_t cpol;
    uint8_t size;   // data size from 4 to 16 bit
    bool io_mode;
    SPI_ORDER order;
    SPI_SPEED speed;
}SPI_MODE;

#define SPI_IO_MODE               ( 1 << 31)

#define SPI_LSBFIRST_MSK          ( 1 << 30)
#define SPI_CPOL_MSK              ( 1 << 29)
#define SPI_CPHA_MSK              ( 1 << 28)



bool spi_open(int port, SPI_MODE* mode);
void spi_close(int port);
bool spi_send(uint32_t port, uint32_t size, uint16_t* data, uint32_t cs_pin);

#define spi_read(port, io, size)              io_read_exo(HAL_IO_REQ(HAL_SPI, IPC_READ), (port), (io), (size))
#define spi_read_sync(port, io, size)         io_read_sync_exo(HAL_IO_REQ(HAL_SPI, IPC_READ), (port), (io), (size))
#define spi_write(port, io, size)             io_write_exo(HAL_IO_REQ(HAL_SPI, IPC_WRITE), (port) | SPI_IO_MODE, (io), (size))
#define spi_write_sync(port, io, size)        io_write_sync_exo(HAL_IO_REQ(HAL_SPI, IPC_WRITE), (port) | SPI_IO_MODE, (io), (size))


#endif // SPI_H
