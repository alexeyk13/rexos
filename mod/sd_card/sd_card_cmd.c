/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sd_card_cmd.h"
#include "sd_card_defs.h"
#include "sdio.h"
#include "dbg.h"
#include "printf.h"
#include "kernel_config.h"
#include "gpio.h"
#include "delay.h"

#if (SD_CARD_DEBUG_ERRORS)
void dbg_r1_error(SD_CARD* sd_card, uint8_t cmd)
{
	if (sd_card->sdio_cmd_response[0] & (1ul << 31ul))
		printf("SD_CARD %d: R1 out of range\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 30ul))
		printf("SD_CARD %d: R1 address error\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 29ul))
		printf("SD_CARD %d: R1 block len error\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 28ul))
		printf("SD_CARD %d: R1 erase seq error\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 27ul))
		printf("SD_CARD %d: R1 erase param error\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 26ul))
		printf("SD_CARD %d: R1 wp violation\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 25ul))
		printf("SD_CARD %d: R1 card is locked\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 24ul))
		printf("SD_CARD %d: R1 lock unlock failed\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 23ul))
		printf("SD_CARD %d: R1 crc error\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 22ul))
		printf("SD_CARD %d: R1 illegal command\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 21ul))
		printf("SD_CARD %d: R1 ecc failed\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 20ul))
		printf("SD_CARD %d: R1 cc error\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 19ul))
		printf("SD_CARD %d: R1 general error\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 16ul))
		printf("SD_CARD %d: R1 CSD overwrite\n\r", cmd);
	if (sd_card->sdio_cmd_response[0] & (1ul << 3ul))
		printf("SD_CARD %d: R1 AKE SEQ error\n\r", cmd);

	switch ((sd_card->sdio_cmd_response[0] >> 9) & 0xf)
	{
	case SD_CARD_STATE_IDLE:
		printf("SD_CARD: state idle\n\r");
		break;
	case SD_CARD_STATE_READY:
		printf("SD_CARD: state ready\n\r");
		break;
	case SD_CARD_STATE_IDENT:
		printf("SD_CARD: state ident\n\r");
		break;
	case SD_CARD_STATE_STBY:
		printf("SD_CARD: state stby\n\r");
		break;
	case SD_CARD_STATE_TRAN:
		printf("SD_CARD: state tran\n\r");
		break;
	case SD_CARD_STATE_DATA:
		printf("SD_CARD: state data\n\r");
		break;
	case SD_CARD_STATE_RCV:
		printf("SD_CARD: state rcv\n\r");
		break;
	case SD_CARD_STATE_PRG:
		printf("SD_CARD: state prg\n\r");
		break;
	case SD_CARD_STATE_DIS:
		printf("SD_CARD: state dis\n\r");
		break;
	}
}
#endif

static inline bool sd_card_cmd(SD_CARD* sd_card, uint8_t cmd, uint32_t arg, SDIO_RESPONSE_TYPE response_type)
{
	switch (sdio_cmd(sd_card->port, cmd, sd_card->sdio_cmd_response, arg, response_type))
	{
	case SDIO_RESULT_OK:
		return true;
		break;
	case SDIO_RESULT_TIMEOUT:
		sd_card->status = STORAGE_STATUS_TIMEOUT;
		break;
	case SDIO_RESULT_CRC_FAIL:
		sd_card->status = STORAGE_STATUS_CRC_ERROR;
		break;
	}
	return false;
}

static inline bool sd_card_r1_cmd(SD_CARD* sd_card, uint8_t cmd, uint32_t arg)
{
	if (sd_card_cmd(sd_card, cmd, arg, SDIO_RESPONSE_R1))
	{
		//due to specification, OUT_OF_RANGE error for last block reading must be ignored
		//but f*cking A-Data manufacturer return OUT_OF_RANGE every time, 8 sectors is requested
		if ((sd_card->sdio_cmd_response[0] & SDIO_R1_LAST_ERROR_BITS) == 0)
			return true;
		else
		{
			if ((sd_card->sdio_cmd_response[0] & (1ul << 31ul)) || (sd_card->sdio_cmd_response[0] & (1ul << 30ul)))
				sd_card->status = STORAGE_STATUS_INVALID_ADDRESS;
			else if (sd_card->sdio_cmd_response[0] & (1ul << 26ul))
				sd_card->status = STORAGE_STATUS_DATA_PROTECTED;
			else if ((sd_card->sdio_cmd_response[0] & (1ul << 23ul)) || (sd_card->sdio_cmd_response[0] & (1ul << 21ul)))
				sd_card->status = STORAGE_STATUS_CRC_ERROR;
			else
				sd_card->status = STORAGE_STATUS_HARDWARE_FAILURE;
#if (SD_CARD_DEBUG_ERRORS)
			dbg_r1_error(sd_card, cmd);
#endif
		}
	}
	return false;
}

bool sd_card_app_cmd(SD_CARD* sd_card, uint8_t cmd, uint32_t arg, SDIO_RESPONSE_TYPE response_type)
{
	bool res = false;

	if (sd_card_r1_cmd(sd_card, SDIO_APP_CMD, (uint32_t)sd_card->rca << 16ul) && (sd_card->sdio_cmd_response[0] & SDIO_R1_BIT_APP_CMD))
		//ready to process app cmd
		res = sd_card_cmd(sd_card, cmd, arg, response_type);
	return res;
}

static inline bool sd_card_wait_for_ready(SD_CARD* sd_card)
{
	bool res = false;
	while (!res)
	{
		if (sd_card_r1_cmd(sd_card, SDIO_SEND_STATUS, (uint32_t)sd_card->rca << 16ul))
		{
			if (sd_card->sdio_cmd_response[0] & (1 << 8ul))
				res = true;
		}
		else
			break;
	}
	return res;
}

static inline bool sd_card_wait_for_data_transferred(SD_CARD* sd_card)
{
	//recovery, based on TOSHIBA recomendations
	int retries = 1000 * 10;
	while (retries--)
	{
		if ((sd_card_r1_cmd(sd_card, SDIO_SEND_STATUS, (uint32_t)sd_card->rca << 16ul)))
		{
			if (((sd_card->sdio_cmd_response[0] >> 9) & 0xf) == SD_CARD_STATE_TRAN)
			{
				sd_card->status = STORAGE_STATUS_OK;
				return true;
			}
		}
		delay_us(100);
	}
	return false;
}

static inline bool sd_card_check_power_conditions(SD_CARD* sd_card)
{
	bool res = false;
	bool v2 = false;
	int retry_count = 1100;
	uint32_t voltage_window = SD_VOLTAGE_WINDOW;
	if (sd_card_cmd(sd_card, SDIO_GO_IDLE_STATE, 0x00, SDIO_NO_RESPONSE))
	{
		if (sd_card_cmd(sd_card, SDIO_SEND_IF_COND, SDIO_CHECK_PATTERN, SDIO_RESPONSE_R7))
		{
			if ((sd_card->sdio_cmd_response[0] & 0xff) == (SDIO_CHECK_PATTERN & 0xff) && (sd_card->sdio_cmd_response[0] & 0x100))
			{
				v2 = true;
				voltage_window |= SDIO_R3_HCS;
			}
		}

		while (!res && retry_count &&	sd_card_app_cmd(sd_card, SDIO_APP_SD_SEND_OP_COND, voltage_window, SDIO_RESPONSE_R3))
		{
			if (sd_card->sdio_cmd_response[0] & SDIO_R3_INIT_DONE)
				res = true;
			else
			{
				--retry_count;
				delay_ms(1);
			}
		}
	}
	if (res)
	{
		if (v2)
		{

#if (SD_CARD_DEBUG_FLOW)
			printf("SD_CARD OCR: %08x\n\r", sd_card->sdio_cmd_response[0]);
#endif
			if (sd_card->sdio_cmd_response[0] & SDIO_R3_CCS)
				sd_card->card_type = SDHC_V2;
			else
				sd_card->card_type = SDSC_V2;
		}
		else
			sd_card->card_type = SDSC_V1;
	}
	return res;
}

static inline bool sd_card_read_cid(SD_CARD* sd_card)
{
	bool res = false;
	if (sd_card_cmd(sd_card, SDIO_ALL_SEND_CID, 0x00, SDIO_RESPONSE_R2))
	{
		//MID + MDT + PSN
		sprintf(sd_card->media.serial_number, "%02X%02X%06X%02X", sd_card->sdio_cmd_response[0] >> 24,
				  (sd_card->sdio_cmd_response[3] >> 16) & 0xff, sd_card->sdio_cmd_response[2] >> 8, sd_card->sdio_cmd_response[3] >> 24);
#if (SD_CARD_DEBUG_FLOW)
		printf("SD_CARD CID.MID: %#.2x\n\r", sd_card->sdio_cmd_response[0] >> 24);
		printf("SD_CARD CID.OID: %c%c\n\r", (char)(sd_card->sdio_cmd_response[0] >> 16), (char)(sd_card->sdio_cmd_response[0] >> 8));
		printf("SD_CARD CID.PNM: %c%c%c%c%c\n\r", (char)(sd_card->sdio_cmd_response[0] >> 0), (char)(sd_card->sdio_cmd_response[1] >> 24),
				(char)(sd_card->sdio_cmd_response[1] >> 16), (char)(sd_card->sdio_cmd_response[1] >> 8), (char)(sd_card->sdio_cmd_response[1] >> 0));
		printf("SD_CARD CID.PRV: %x.%x\n\r", sd_card->sdio_cmd_response[2] >> 28, (sd_card->sdio_cmd_response[2] >> 24) & 0xf);
		printf("SD_CARD CID.PSN: %.8x\n\r", (sd_card->sdio_cmd_response[2] << 4) | (sd_card->sdio_cmd_response[3] >> 24));
		printf("SD_CARD CID.MDT: %d,%d\n\r", (sd_card->sdio_cmd_response[3] >> 8) & 0xf, ((sd_card->sdio_cmd_response[3] >> 12) & 0xff) + 2000);
#endif
		res = true;
	}
	return res;
}

static inline bool sd_card_read_rca(SD_CARD* sd_card)
{
	bool res = false;
	if (sd_card_cmd(sd_card, SDIO_SEND_RELATIVE_ADDR, 0x00, SDIO_RESPONSE_R6))
	{
		sd_card->rca = (uint16_t)(sd_card->sdio_cmd_response[0] >> 16ul);
#if (SD_CARD_DEBUG_FLOW)
		printf("SD_CARD RCA: %04x\n\r", sd_card->rca);
#endif
		res = true;
	}
	return true;
}

static inline bool sd_card_read_csd(SD_CARD* sd_card)
{
	bool res = false;

	if (sd_card_cmd(sd_card, SDIO_SEND_CSD, (uint32_t)sd_card->rca << 16ul, SDIO_RESPONSE_R2))
	{
#if (SD_CARD_DEBUG_FLOW)
		printf("SD_CARD CSD: %08x %08x %08x %08x\n\r", sd_card->sdio_cmd_response[0], sd_card->sdio_cmd_response[1], sd_card->sdio_cmd_response[2], sd_card->sdio_cmd_response[3]);
#endif
		uint8_t csd_version = sd_card->sdio_cmd_response[0] >> 30;
		//csd v1, csd v2
		if (csd_version < 2)
		{
			//sector size
			if (((sd_card->sdio_cmd_response[1] >> 16) & 0xf) == 9)
			{
				//== v2
				if (csd_version == 1)
				{
					uint32_t c_size = ((sd_card->sdio_cmd_response[1] & 0x3ful) << 16ul) | (sd_card->sdio_cmd_response[2] >> 16ul);
					sd_card->media.num_sectors = (c_size + 1) * 1024;
				}
				else
				{
					uint32_t mult = 1ul << (((sd_card->sdio_cmd_response[2] >> 15ul) & 0x7)  + 2ul);
					uint32_t c_size = ((sd_card->sdio_cmd_response[1] & 0x3fful) << 2ul) | (sd_card->sdio_cmd_response[2] >> 30ul);
					sd_card->media.num_sectors = (c_size + 1) * mult;
				}
				sd_card->media.flags = 0;
				if ((sd_card->sdio_cmd_response[3] >> 12) & 3ul)
					sd_card->media.flags |= STORAGE_MEDIA_FLAG_WRITE_PROTECTION;
				res = true;
			}
		}
	}
	return res;
}

static inline bool sd_card_get_registers(SD_CARD* sd_card)
{
	bool res = false;
	if (sd_card_read_cid(sd_card))
		if (sd_card_read_rca(sd_card))
			if (sd_card_read_csd(sd_card))
				res = true;
	return res;
}

static inline bool sd_card_select(SD_CARD* sd_card)
{
	bool res = false;

	if (sd_card_r1_cmd(sd_card, SDIO_SELECT_DESELECT_CARD, (uint32_t)sd_card->rca << 16ul))
			res = true;
	return res;
}

static inline bool sd_card_read_scr(SD_CARD* sd_card)
{
	bool res = false;

	if (sd_card_wait_for_ready(sd_card))
		if (sd_card_r1_cmd(sd_card, SDIO_SET_BLOCKLEN, 8))
		{
			sd_card->status = STORAGE_STATUS_OK;
			sd_card->futex = false;
			sdio_read(sd_card->port, sd_card->cmd_buf, 8, 1);
			if (sd_card_app_cmd(sd_card, SDIO_APP_SEND_SCR, 0x0, SDIO_RESPONSE_R1))
			{
				while (!sd_card->futex) {}
				if (sd_card->status == STORAGE_STATUS_OK)
				{
					sd_card_wait_for_data_transferred(sd_card);
#if (SD_CARD_DEBUG_FLOW)
					printf("SD_CARD SCR: %08x %08x\n\r", ((unsigned int*)sd_card->cmd_buf[0], ((unsigned int*)sd_card->cmd_buf[1]);
#endif
					res = true;
				}
			}
		}
	return res;
}

static inline bool sd_card_switch(SD_CARD* sd_card, uint32_t cmd)
{
	bool res = false;
	if (sd_card_r1_cmd(sd_card, SDIO_SWITCH_FUNC, cmd))
	{
		sd_card->status = STORAGE_STATUS_OK;
		sd_card->futex = false;
		sdio_read(sd_card->port, sd_card->cmd_buf, 64, 1);
		while (!sd_card->futex) {}
		if (sd_card->status == STORAGE_STATUS_OK)
		{
			sd_card_wait_for_data_transferred(sd_card);
			res = true;
		}
	}
	return res;
}

bool sd_card_setup_bus(SD_CARD* sd_card)
{
	bool res = false;
	if (sd_card_read_scr(sd_card))
	{
		res = true;
		//bus width 4 bit support bit
		if (sd_card->cmd_buf[1] & 4)
			if (sd_card_app_cmd(sd_card, SDIO_APP_SET_BUS_WIDTH, 0x2, SDIO_RESPONSE_R1))
			{
				delay_ms(100);
				sdio_setup_bus(sd_card->port, 25000000, SDIO_BUS_WIDE_4B);
			}
		//check for CMD6 support
		if (sd_card->cmd_buf[0] & 0xf)
			if (sd_card_switch(sd_card, SDIO_SWITCH_HIGH_SPEED_CHECK))
				if (sd_card->cmd_buf[0xc] & 0x80)
				{
#if (SD_CARD_DEBUG)
					printf("SD_CARD: HIGH SPEED mode supported\n\r");
#endif
					sd_card_switch(sd_card, SDIO_SWITCH_CURRENT_400);
					sd_card_switch(sd_card, SDIO_SWITCH_STRENGTH_A);
					sd_card_switch(sd_card, SDIO_SWITCH_COMMAND_EC);
					sd_card_switch(sd_card, SDIO_SWITCH_HIGH_SPEED);
					sdio_setup_bus(sd_card->port, 50000000, SDIO_BUS_WIDE_4B);
				}
	}
	delay_ms(100);
	return res;
}

static inline bool sd_card_validate(SD_CARD* sd_card)
{
	bool res = false;
	sd_card->rca = 0;
	sdio_power_on(sd_card->port);
	sdio_setup_bus(sd_card->port, 400000, SDIO_BUS_WIDE_1B);

	if (sd_card_check_power_conditions(sd_card))
		if (sd_card_get_registers(sd_card))
		{
			//at this point we are ready to faster speed to 25MHz
			sdio_setup_bus(sd_card->port, 25000000, SDIO_BUS_WIDE_1B);
			if (sd_card_select(sd_card))
				if (sd_card_setup_bus(sd_card))
				{
					//SDSC requires block len
					if (sd_card_wait_for_ready(sd_card))
						res = sd_card_r1_cmd(sd_card, SDIO_SET_BLOCKLEN, SD_CARD_SECTOR_SIZE);
				}
		}
	if (!res)
		sdio_power_off(sd_card->port);
	return res;
}

bool sd_card_check_media(void* param)
{
	SD_CARD* sd_card = (SD_CARD*)param;

#if (SD_CARD_CD_PIN_ENABLED)
	if (!gpio_get_pin(SD_CARD_CD_PIN))
	{
#endif
		if (!sd_card->card_present)
			if (sd_card_validate(sd_card))
			{
#if (SD_CARD_DEBUG)
				switch(sd_card->card_type)
				{
				case SDSC_V1:
					printf("SD_CARD: found SD v1\n\r");
					break;
				case SDSC_V2:
					printf("SD_CARD: found SD v2\n\r");
					break;
				case SDHC_V2:
					printf("SD_CARD: found SDHC v2\n\r");
					break;
				}
#endif
				sd_card->card_present = true;
				storage_insert_media(sd_card->storage, &sd_card->media);
			}
#if (SD_CARD_CD_PIN_ENABLED)
	}
	else if (sd_card->card_present)
	{
		sdio_power_off(sd_card->port);
		sd_card->card_present = false;
		storage_eject_media(sd_card->storage);
	}
#endif
	return sd_card->card_present;
}

void sd_card_stop(void* param)
{
	SD_CARD* sd_card = (SD_CARD*)param;
	if (sd_card->card_present)
	{
		if (sd_card_wait_for_ready(sd_card))
			sd_card_cmd(sd_card, SDIO_GO_INACTIVE_STATE, (uint32_t)sd_card->rca << 16ul, SDIO_NO_RESPONSE);
		sd_card->rca = 0;
		sd_card->card_present = false;
		sdio_power_off(sd_card->port);
	}
}

void sd_card_on_read_complete(SDIO_CLASS port, void* param)
{
	SD_CARD* sd_card = (SD_CARD*)param;
	if (sd_card->card_present)
	{
		sd_card_r1_cmd(sd_card, SDIO_STOP_TRANSMISSION, 0x0);
		storage_blocks_readed(sd_card->storage, sd_card->status);
	}
	//scr read, capacity check io read
	else
		sd_card->futex = true;
}

void sd_card_on_write_complete(SDIO_CLASS port, void* param)
{
	SD_CARD* sd_card = (SD_CARD*)param;
	if (sd_card->card_present)
	{
		sd_card_r1_cmd(sd_card, SDIO_STOP_TRANSMISSION, 0x0);
		storage_blocks_writed(sd_card->storage, sd_card->status);
	}
}

void sd_card_on_error(SDIO_CLASS port, void* param, SDIO_ERROR error)
{
	SD_CARD* sd_card = (SD_CARD*)param;
	sd_card_r1_cmd(sd_card, SDIO_STOP_TRANSMISSION, 0x0);
	if (sd_card->card_present)
	{
		switch(error)
		{
		case SDIO_ERROR_TIMEOUT:
			sd_card->status = STORAGE_STATUS_TIMEOUT;
			break;
		case SDIO_ERROR_CRC:
			sd_card->status = STORAGE_STATUS_CRC_ERROR;
			break;
		default:
			sd_card->status = STORAGE_STATUS_HARDWARE_FAILURE;
		}
		if (sd_card->writing)
			storage_blocks_writed(sd_card->storage, sd_card->status);
		else
			storage_blocks_readed(sd_card->storage, sd_card->status);
	}
	//scr or capacity check request failed
	else
		sd_card->futex = true;
}

STORAGE_STATUS sd_card_read_blocks(void* param, unsigned long addr, char* buf, unsigned long blocks_count)
{
	SD_CARD* sd_card = (SD_CARD*)param;
	sd_card->status = STORAGE_STATUS_OK;
	sd_card->writing = false;
	if (sd_card_check_media(param))
	{
		if (sd_card_wait_for_data_transferred(sd_card))
		{
			//SDSC cards operates in absolute addr units
			if (sd_card->card_type == SDSC_V1 || sd_card->card_type == SDSC_V2)
				addr = addr * SD_CARD_SECTOR_SIZE;

			if (sd_card_r1_cmd(sd_card, SDIO_READ_MULTIPLE_BLOCK, addr))
				sdio_read(sd_card->port, buf, SD_CARD_SECTOR_SIZE, blocks_count);
		}
		else
			storage_blocks_readed(sd_card->storage, sd_card->status);
	}
	else
		sd_card->status = STORAGE_STATUS_NO_MEDIA;
	return sd_card->status;
}

STORAGE_STATUS sd_card_write_blocks(void* param, unsigned long addr, char* buf, unsigned long blocks_count)
{
	SD_CARD* sd_card = (SD_CARD*)param;
	sd_card->status = STORAGE_STATUS_OK;
	sd_card->writing = true;
	if (sd_card_check_media(sd_card))
	{
		if (sd_card_wait_for_data_transferred(sd_card))
		{
			//SDSC cards operates in absolute addr units
			if (sd_card->card_type == SDSC_V1 || sd_card->card_type == SDSC_V2)
				addr = addr * SD_CARD_SECTOR_SIZE;

			if (sd_card_r1_cmd(sd_card, SDIO_WRITE_MULTIPLE_BLOCK, addr))
				sdio_write(sd_card->port, buf, SD_CARD_SECTOR_SIZE, blocks_count);
		}
		else
			storage_blocks_writed(sd_card->storage, sd_card->status);
	}
	else
		sd_card->status = STORAGE_STATUS_NO_MEDIA;
	return sd_card->status;
}
