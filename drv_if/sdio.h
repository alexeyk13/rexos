/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SDIO_H
#define SDIO_H

#include "types.h"
#include "dev.h"

typedef enum {
	SDIO_NO_RESPONSE,
	SDIO_RESPONSE_R1,
	SDIO_RESPONSE_R1B,
	SDIO_RESPONSE_R2,
	SDIO_RESPONSE_R3,
	SDIO_RESPONSE_R4,
	SDIO_RESPONSE_R4B,
	SDIO_RESPONSE_R5,
	SDIO_RESPONSE_R6,
	SDIO_RESPONSE_R7
}SDIO_RESPONSE_TYPE;

typedef enum {
	SDIO_RESULT_OK,
	SDIO_RESULT_TIMEOUT,
	SDIO_RESULT_CRC_FAIL
}SDIO_RESULT;

typedef enum {
	SDIO_BUS_WIDE_1B = 0,
	SDIO_BUS_WIDE_4B,
	SDIO_BUS_WIDE_8B
}SDIO_BUS_WIDE;

typedef enum {
	SDIO_ERROR_TIMEOUT,
	SDIO_ERROR_CRC,
	SDIO_ERROR_OVERRUN,
	SDIO_ERROR_UNDERRUN
}SDIO_ERROR;

typedef void (*SDIO_HANDLER)(SDIO_CLASS, void*);
typedef void (*SDIO_ERROR_HANDLER)(SDIO_CLASS, void*, SDIO_ERROR);

typedef struct {
	SDIO_HANDLER on_read_complete;
	SDIO_HANDLER on_write_complete;
	SDIO_ERROR_HANDLER on_error;
}SDIO_CB, *P_SDIO_CB;

void sdio_enable(SDIO_CLASS port, SDIO_CB *cb, void* param, int priority);
void sdio_disable(SDIO_CLASS port);
void sdio_power_on(SDIO_CLASS port);
void sdio_power_off(SDIO_CLASS port);
void sdio_setup_bus(SDIO_CLASS port, uint32_t bus_freq, SDIO_BUS_WIDE bus_wide);

//because sdio_cmd max timeout is only 64 ahb, this calls are blocking
SDIO_RESULT sdio_cmd(SDIO_CLASS port, uint8_t cmd, uint32_t* sdio_cmd_response, uint32_t arg, SDIO_RESPONSE_TYPE response_type);
//async. isr will call completer on data transferred/error
void sdio_read(SDIO_CLASS port, char* buf, uint16_t block_size, uint32_t blocks_count);
void sdio_write(SDIO_CLASS port, char* buf, uint16_t block_size, uint32_t blocks_count);

#endif // SDIO_H
