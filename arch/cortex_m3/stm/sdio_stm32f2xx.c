/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sdio.h"
#include "arch.h"
#include "gpio.h"
#include "gpio_stm32.h"
#include "rcc_stm32f2.h"
#include "dma_stm32.h"
#include "sd_card.h"
#include "hw_config.h"
#include "../../../userspace/core/core.h"
#include "error_private.h"
#include "dbg.h"
#include "mem_private.h"
#include "sdio.h"

#define SDIO_D0_PIN							GPIO_C8
#define SDIO_D1_PIN							GPIO_C9
#define SDIO_D2_PIN							GPIO_C10
#define SDIO_D3_PIN							GPIO_C11
#define SDIO_CMD_PIN							GPIO_D2
#define SDIO_CLK_PIN							GPIO_C12

#define SDIO_STATIC_FLAGS					0x00c007ff

#define SDIO_POWER_ON						(3 << 0)
#define SDIO_POWER_OFF						(0 << 0)

#define SDIO_CLKCR_WIDBUS_1B				(0 << 11)
#define SDIO_CLKCR_WIDBUS_4B				(1 << 11)
#define SDIO_CLKCR_WIDBUS_8B				(2 << 11)

#define SDIO_CMD_WAITRESP_NO				(0 << 6)
#define SDIO_CMD_WAITRESP_SHORT			(1 << 6)
#define SDIO_CMD_WAITRESP_LONG			(3 << 6)

#define SDIO_DCTRL_DBLOCKSIZE_POS		(4)

typedef struct {
    SDIO_CB* cb;
	void* param;
	unsigned int read_dtimer, write_dtimer;
}SDIO_HW;

static SDIO_HW* _sdio_hw[1] =							{NULL};

void SDIO_IRQHandler(void)
{
	uint32_t sta = SDIO->STA;
	SDIO->ICR = SDIO_STATIC_FLAGS;
	SDIO->MASK = 0;
	if (sta & SDIO_STA_DATAEND)
	{
		if (SDIO->DCTRL & SDIO_DCTRL_DTDIR)
			_sdio_hw[SDIO_1]->cb->on_read_complete(SDIO_1, _sdio_hw[SDIO_1]->param);
		else
			_sdio_hw[SDIO_1]->cb->on_write_complete(SDIO_1, _sdio_hw[SDIO_1]->param);
	}
	else if (sta & SDIO_STA_DTIMEOUT)
		_sdio_hw[SDIO_1]->cb->on_error(SDIO_1, _sdio_hw[SDIO_1]->param, SDIO_ERROR_TIMEOUT);
	else if (sta & SDIO_STA_DCRCFAIL)
		_sdio_hw[SDIO_1]->cb->on_error(SDIO_1, _sdio_hw[SDIO_1]->param, SDIO_ERROR_CRC);
}

void sdio_enable(SDIO_CLASS port, SDIO_CB *cb, void *param, int priority)
{
    if (port < 1)
	{
		SDIO_HW* sdio_hw = (SDIO_HW*)sys_alloc(sizeof(SDIO_HW));
		if (sdio_hw)
		{
			sdio_hw->cb = cb;
			sdio_hw->param = param;
			sdio_hw->read_dtimer = 0xffffffff;
			sdio_hw->write_dtimer = 0xffffffff;
			_sdio_hw[port] = sdio_hw;

			//enable pins
			gpio_enable_afio(SDIO_D0_PIN, AFIO_MODE_FSMC_SDIO_OTG_FS_PULL_UP);
			gpio_enable_afio(SDIO_D1_PIN, AFIO_MODE_FSMC_SDIO_OTG_FS_PULL_UP);
			gpio_enable_afio(SDIO_D2_PIN, AFIO_MODE_FSMC_SDIO_OTG_FS_PULL_UP);
			gpio_enable_afio(SDIO_D3_PIN, AFIO_MODE_FSMC_SDIO_OTG_FS_PULL_UP);
			gpio_enable_afio(SDIO_CMD_PIN, AFIO_MODE_FSMC_SDIO_OTG_FS_PULL_UP);
			gpio_enable_afio(SDIO_CLK_PIN, AFIO_MODE_FSMC_SDIO_OTG_FS);

			//enable clock
			RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;
			// Enable the DMA2 Clock
			dma_enable(DMA_2);
			DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].FCR = DMA_STREAM_FCR_FTH_FULL | DMA_SxFCR_DMDIS;
			DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].PAR = (uint32_t)&(SDIO->FIFO);

			//enable interrupts
			NVIC_EnableIRQ(SDIO_IRQn);
			NVIC_SetPriority(SDIO_IRQn, priority);
		}
		else
			error_dev(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, DEV_SDIO, port);
	}
	else
		error_dev(ERROR_DEVICE_INDEX_OUT_OF_RANGE, DEV_SDIO, port);
}

void sdio_disable(SDIO_CLASS port)
{
    if (port < 1)
	{
		NVIC_DisableIRQ(SDIO_IRQn);
		//disable clock
		RCC->APB2ENR &= ~RCC_APB2ENR_SDIOEN;
		//disable DMA
		dma_disable(DMA_2);
		//disable pins
		gpio_disable_pin(SDIO_D0_PIN);
		gpio_disable_pin(SDIO_D1_PIN);
		gpio_disable_pin(SDIO_D2_PIN);
		gpio_disable_pin(SDIO_D3_PIN);
		gpio_disable_pin(SDIO_CLK_PIN);
		gpio_disable_pin(SDIO_CMD_PIN);
		sys_free(_sdio_hw[port]);
		_sdio_hw[port] = NULL;
	}
	else
		error_dev(ERROR_DEVICE_INDEX_OUT_OF_RANGE, DEV_SDIO, port);
}

void sdio_power_on(SDIO_CLASS port)
{
	SDIO->POWER = SDIO_POWER_ON;
	SDIO->CLKCR |= SDIO_CLKCR_CLKEN;
}

void sdio_power_off(SDIO_CLASS port)
{
	SDIO->CLKCR &= ~SDIO_CLKCR_CLKEN;
	SDIO->POWER = SDIO_POWER_OFF;
}

void sdio_setup_bus(SDIO_CLASS port, uint32_t bus_freq, SDIO_BUS_WIDE bus_wide)
{
	SDIO->CLKCR &= SDIO_CLKCR_HWFC_EN | SDIO_CLKCR_NEGEDGE | SDIO_CLKCR_PWRSAV | SDIO_CLKCR_CLKEN | SDIO_CLKCR_BYPASS;
	uint32_t divisor, real_freq;
	//activate bypass mode for divisor 1
	if (_fs_freq <= bus_freq)
	{
		SDIO->CLKCR |= SDIO_CLKCR_BYPASS;
		real_freq = _fs_freq;
	}
	else
	{
		divisor = _fs_freq / bus_freq;
		if (divisor < 2)
			divisor = 2;
		real_freq = _fs_freq / divisor;
		SDIO->CLKCR |= (divisor - 2) & 0xff;
	}
	SDIO->CLKCR |= bus_wide == SDIO_BUS_WIDE_8B ? SDIO_CLKCR_WIDBUS_8B : bus_wide == SDIO_BUS_WIDE_4B ? SDIO_CLKCR_WIDBUS_4B : SDIO_CLKCR_WIDBUS_1B;
	//100 ms
	_sdio_hw[port]->read_dtimer = real_freq / 10;
	//250 ms
	_sdio_hw[port]->write_dtimer = real_freq / 4;
}

SDIO_RESULT sdio_cmd(SDIO_CLASS port, uint8_t cmd, uint32_t* sdio_cmd_response, uint32_t arg, SDIO_RESPONSE_TYPE response_type)
{
	SDIO_RESULT res = SDIO_RESULT_OK;

	SDIO->ARG = arg;
	SDIO->CMD = (cmd & 0x3f) | (response_type == SDIO_NO_RESPONSE ? SDIO_CMD_WAITRESP_NO : response_type == SDIO_RESPONSE_R2 ?
					SDIO_CMD_WAITRESP_LONG : SDIO_CMD_WAITRESP_SHORT);
	SDIO->CMD |= SDIO_CMD_CPSMEN;

	if (response_type == SDIO_NO_RESPONSE)
		while ((SDIO->STA & SDIO_STA_CMDSENT) == 0) {}
	else
	{
		while ((SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL)) == 0) {}
		if (SDIO->STA & SDIO_STA_CTIMEOUT)
			res = SDIO_RESULT_TIMEOUT;
		//response R3 doesn't contains CRC
		else if ((SDIO->STA & SDIO_STA_CCRCFAIL) && (response_type != SDIO_RESPONSE_R3))
			res = SDIO_RESULT_CRC_FAIL;
		else
		{
			sdio_cmd_response[0] = SDIO->RESP1;
			if (response_type == SDIO_RESPONSE_R2)
			{
				sdio_cmd_response[1] = SDIO->RESP2;
				sdio_cmd_response[2] = SDIO->RESP3;
				sdio_cmd_response[3] = SDIO->RESP4;
			}
		}
	}
	SDIO->ICR = SDIO_STATIC_FLAGS;
	return res;
}

static inline uint8_t pow_2(uint16_t val)
{
	uint8_t res;
	uint16_t tmp = val;
	for (res = 0; res < 16 && tmp; ++res)
		tmp = tmp >> 1;

	return res - 1;
}

void sdio_read(SDIO_CLASS port, char* buf, uint16_t block_size, uint32_t blocks_count)
{
	SDIO->DTIMER = _sdio_hw[port]->read_dtimer;
	SDIO->DLEN = block_size * blocks_count;
	//from card
	SDIO->DCTRL = (pow_2(block_size) << SDIO_DCTRL_DBLOCKSIZE_POS) | SDIO_DCTRL_DTDIR;
	SDIO->ICR = SDIO_STATIC_FLAGS;

	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].CR = DMA_STREAM_CR_CHANNEL_4 | DMA_STREAM_CR_MBURST_INCR4 | DMA_STREAM_CR_PBURST_INCR4 | DMA_STREAM_CR_PL_VERY_HIGH |
															DMA_STREAM_CR_MSIZE_WORD | DMA_STREAM_CR_PSIZE_WORD | DMA_SxCR_MINC | DMA_STREAM_CR_DIR_P2M | DMA_SxCR_PFCTRL;
	dma_clear_flags(DMA_2, SDIO_DMA_STREAM, DMA_FEIF | DMA_DMEIF | DMA_TEIF | DMA_HTIF | DMA_TCIF);

	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].NDTR = block_size * blocks_count / 4;
	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].M0AR = (uint32_t)buf;
	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].CR |= DMA_SxCR_EN;
	SDIO->DCTRL |= SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
	SDIO->MASK |= SDIO_MASK_DATAENDIE | SDIO_MASK_DTIMEOUTIE | SDIO_MASK_DCRCFAILIE;
}

void sdio_write(SDIO_CLASS port, char* buf, uint16_t block_size, uint32_t blocks_count)
{
	SDIO->DTIMER = _sdio_hw[port]->write_dtimer;
	SDIO->DLEN = block_size * blocks_count;
	//to card
	SDIO->DCTRL = (pow_2(block_size) << SDIO_DCTRL_DBLOCKSIZE_POS);
	SDIO->ICR = SDIO_STATIC_FLAGS;

	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].CR = DMA_STREAM_CR_CHANNEL_4 | DMA_STREAM_CR_MBURST_INCR4 | DMA_STREAM_CR_PBURST_INCR4 | DMA_STREAM_CR_PL_VERY_HIGH |
															DMA_STREAM_CR_MSIZE_WORD | DMA_STREAM_CR_PSIZE_WORD | DMA_SxCR_MINC | DMA_STREAM_CR_DIR_M2P | DMA_SxCR_PFCTRL;
	dma_clear_flags(DMA_2, SDIO_DMA_STREAM, DMA_FEIF | DMA_DMEIF | DMA_TEIF | DMA_HTIF | DMA_TCIF);

	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].NDTR = block_size * blocks_count / 4;
	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].M0AR = (uint32_t)buf;

	DMA[DMA_2]->STREAM[SDIO_DMA_STREAM].CR |= DMA_SxCR_EN;
	SDIO->DCTRL |= SDIO_DCTRL_DMAEN;

	SDIO->DCTRL |= SDIO_DCTRL_DTEN;
	SDIO->MASK |= SDIO_MASK_DATAENDIE | SDIO_MASK_DTIMEOUTIE | SDIO_MASK_DCRCFAILIE;
}
