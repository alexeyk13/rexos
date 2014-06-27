/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "scsi.h"
#include "dbg.h"
#include "kernel_config.h"
#include <string.h>
#include "scsi_io.h"
#include "scsi_private.h"
#include "scsi_page.h"
#include "queue.h"
#include "mem_private.h"
#include "error.h"

void scsi_error(SCSI* scsi, uint8_t code, uint16_t asq)
{
	//error queue overflow
	if (((scsi->error_head + 1) & SCSI_ERROR_BUF_MASK) == scsi->error_tail)
	{
		//remove TWO items, post error code
		scsi->error_tail = (scsi->error_tail + 2) & SCSI_ERROR_BUF_MASK;

		scsi->error_queue[scsi->error_head].code = SENSE_KEY_NO_SENSE;
		scsi->error_queue[scsi->error_head].asq = ASQ_ERROR_LOG_OVERFLOW;
		scsi->error_head = (scsi->error_head + 1) & SCSI_ERROR_BUF_MASK;
	}
	scsi->error_queue[scsi->error_head].code = code;
	scsi->error_queue[scsi->error_head].asq = asq;
	scsi->error_head = (scsi->error_head + 1) & SCSI_ERROR_BUF_MASK;

#if (SCSI_DEBUG_ERRORS)
	//ignore errors:
	//- about no media present
	//- unsupported
	if (storage_is_media_present(scsi->storage) && asq != ASQ_INVALID_COMMAND_OPERATION_CODE)
		printf("SCSI report error 0x%02X, ASQ: 0x%04X, opcode: 0x%02X\n\r", code, asq, scsi->cmd.opcode);
#if (SCSI_DEBUG_UNSUPPORTED)
	if (asq == ASQ_INVALID_COMMAND_OPERATION_CODE)
		printf("SCSI unsupported opcode: 0x%02X\n\r", scsi->cmd.opcode);
#endif //SCSI_DEBUG_UNSUPPORTED

#endif //SCSI_DEBUG_ERRORS
}

static inline void scsi_decode_cmd(SCSI* scsi, char* cmd_buf, int cmd_size)
{
	scsi->cmd.cmd_type = cmd_size;
	scsi->cmd.opcode = cmd_buf[0];
	scsi->cmd.flags = cmd_buf[1];
	scsi->cmd.control = cmd_buf[cmd_size -1];
	switch (scsi->cmd.cmd_type)
	{
	case SCSI_CMD_6:
		scsi->cmd.address = ((uint16_t)cmd_buf[2] << 8) | ((uint16_t)cmd_buf[3]);
		scsi->cmd.len = cmd_buf[4];
		scsi->cmd.misc = 0;
		scsi->cmd.additional_data = 0;
		break;
	case SCSI_CMD_10:
		scsi->cmd.address = ((uint32_t)cmd_buf[2] << 24) | ((uint32_t)cmd_buf[3] << 16) | ((uint32_t)cmd_buf[4] << 8) | ((uint32_t)cmd_buf[5]);
		scsi->cmd.len = ((uint16_t)cmd_buf[7] << 8) | ((uint16_t)cmd_buf[8]);
		scsi->cmd.misc = cmd_buf[6];
		scsi->cmd.additional_data = 0;
		break;
	case SCSI_CMD_12:
		scsi->cmd.flags = cmd_buf[1];
		scsi->cmd.address = ((uint32_t)cmd_buf[2] << 24) | ((uint32_t)cmd_buf[3] << 16) | ((uint32_t)cmd_buf[4] << 8) | ((uint32_t)cmd_buf[5]);
		scsi->cmd.len = ((uint32_t)cmd_buf[6] << 24) | ((uint32_t)cmd_buf[7] << 16) | ((uint32_t)cmd_buf[8] << 8) | ((uint32_t)cmd_buf[9]);
		scsi->cmd.misc = cmd_buf[10];
		scsi->cmd.additional_data = 0;
		break;
	case SCSI_CMD_16:
		scsi->cmd.flags = cmd_buf[1];
		scsi->cmd.address = ((uint32_t)cmd_buf[2] << 24) | ((uint32_t)cmd_buf[3] << 16) | ((uint32_t)cmd_buf[4] << 8) | ((uint32_t)cmd_buf[5]);
		scsi->cmd.additional_data = ((uint32_t)cmd_buf[6] << 24) | ((uint32_t)cmd_buf[7] << 16) | ((uint32_t)cmd_buf[8] << 8) | ((uint32_t)cmd_buf[9]);
		scsi->cmd.len = ((uint32_t)cmd_buf[10] << 24) | ((uint32_t)cmd_buf[11] << 16) | ((uint32_t)cmd_buf[12] << 8) | ((uint32_t)cmd_buf[13]);
		scsi->cmd.misc = cmd_buf[14];
		break;
	}
}

static inline bool scsi_cmd_request_sense(SCSI* scsi)
{
	bool res = false;
	//f**ing microsoft suppose, that is SCSI 12 command. WTF???
	if (scsi->cmd.cmd_type == SCSI_CMD_6 || scsi->cmd.cmd_type == SCSI_CMD_12)
	{
		uint8_t code;
		uint16_t	asq;
		if (scsi->error_head != scsi->error_tail)
		{
			code = scsi->error_queue[scsi->error_tail].code;
			asq = scsi->error_queue[scsi->error_tail].asq;
			scsi->error_tail = (scsi->error_tail + 1) & SCSI_ERROR_BUF_MASK;
		}
		//empty queue
		else
		{
			code = SENSE_KEY_NO_SENSE;
			asq = ASQ_NO_ADDITIONAL_SENSE_INFORMATION;
		}
		scsi_fill_error_page(scsi, code, asq);
		res = true;
#if (SCSI_DEBUG_FLOW)
	printf("SCSI request sense 0x%02X, ASQ: 0x%04X\n\r", code, asq);
#endif
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);
	return res;
}

static inline bool scsi_cmd_read6(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
		res = scsi_read(scsi, scsi->cmd.address, scsi->cmd.len ? scsi->cmd.len : 256);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI read6 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len ? scsi->cmd.len : 256);
#endif
	return res;
}

static inline bool scsi_cmd_read10(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
		res = scsi_read(scsi, scsi->cmd.address, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI read10 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif
	return res;
}

static inline bool scsi_cmd_read12(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_12)
		res = scsi_read(scsi, scsi->cmd.address, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI read12 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif
	return res;
}

static inline bool scsi_cmd_read16(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_16)
		res = scsi_read(scsi, scsi->cmd.additional_data, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI read16 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif
	return res;
}

static inline bool  scsi_cmd_prevent_allow_medium_removal(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
	{
		switch (scsi->cmd.len)
		{
		case 0:
		case 1:
			storage_enable_media_removal(scsi->storage);
			res = true;
			break;
		case 2:
		case 3:
			storage_disable_media_removal(scsi->storage);
			res = true;
			break;
		}
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI prevent/allow medium removal: %d\n\r", scsi->cmd.len);
#endif
	return res;
}

static inline bool scsi_cmd_read_capacity(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
	{
		if (storage_check_media(scsi->storage))
		{
			res = true;
			scsi_fill_capacity_page(scsi);
#if (SCSI_DEBUG_FLOW)
	printf("SCSI: read capacity 0x%08X sectors, sector size: %d\n\r", storage_get_media_descriptor(scsi->storage)->num_sectors,
			 storage_get_device_descriptor(scsi->storage)->sector_size);
#endif
		}
		else
			scsi_error(scsi, SENSE_KEY_NOT_READY, ASQ_MEDIUM_NOT_PRESENT);
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);
	return res;
}

static inline bool scsi_cmd_read_format_capacity(SCSI* scsi)
{
	bool res = false;
	//it must be 12, but windows suppose is 10
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
	{
		if (storage_check_media(scsi->storage))
		{
			res = true;
			scsi_fill_format_capacity_page(scsi);
#if (SCSI_DEBUG_FLOW)
	printf("SCSI: read format capacity 0x%08X sectors, sector size: %d\n\r", storage_get_media_descriptor(scsi->storage)->num_sectors,
			 storage_get_device_descriptor(scsi->storage)->sector_size);
#endif
		}
		else
			scsi_error(scsi, SENSE_KEY_NOT_READY, ASQ_MEDIUM_NOT_PRESENT);
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);
	return res;
}

static inline bool scsi_cmd_synchronize_cache(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
	{
		storage_write_cache(scsi->storage);
		res = true;
#if (SCSI_DEBUG_FLOW)
		printf("SCSI: synchronize cache\n\r");
#endif
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);
	return res;
}

static inline bool scsi_cmd_write6(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
		res = scsi_write(scsi, scsi->cmd.address, scsi->cmd.len ? scsi->cmd.len : 256);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI write6 0x%08X, %d sector(s)\n\r", scsi->cmd.address, scsi->cmd.len ? scsi->cmd.len : 256);
#endif //SCSI_DEBUG_FLOW
	return res;
}

static inline bool scsi_cmd_write10(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
		res = scsi_write(scsi, scsi->cmd.address, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI write10 0x%08X, %d sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif //SCSI_DEBUG_FLOW
	return res;
}

static inline bool scsi_cmd_write12(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_12)
		res = scsi_write(scsi, scsi->cmd.address, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI write12 0x%08X, %d sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif //SCSI_DEBUG_FLOW
	return res;
}

static inline bool scsi_cmd_write16(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_16)
		res = scsi_write(scsi, scsi->cmd.additional_data, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI write16 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif
	return res;
}

static inline bool scsi_cmd_verify6(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
		res = scsi_verify(scsi, scsi->cmd.address, scsi->cmd.len ? scsi->cmd.len : 256);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI verify6 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len ? scsi->cmd.len : 256);
#endif
	return res;
}

static inline bool scsi_cmd_verify10(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
		res = scsi_verify(scsi, scsi->cmd.address, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI verify10 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif
	return res;
}

static inline bool scsi_cmd_verify12(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_12)
		res = scsi_verify(scsi, scsi->cmd.address, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI verify12 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif
	return res;
}

static inline bool scsi_cmd_verify16(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_16)
		res = scsi_verify(scsi, scsi->cmd.additional_data, scsi->cmd.len);
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	printf("SCSI verify16 0x%08X, %x sector(s)\n\r", scsi->cmd.address, scsi->cmd.len);
#endif
	return res;
}

static inline bool scsi_cmd_inquiry(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
	{
		//vital data page request
		if (scsi->cmd.flags & SCSI_INQUIRY_EVPD)
		{
			switch (scsi->cmd.address)
			{
			case INQUIRY_VITAL_PAGE_SUPPORTED_PAGES:
				scsi_fill_evpd_page_00(scsi);
				res = true;
				break;
			case INQUIRY_VITAL_PAGE_DEVICE_INFO:
				scsi_fill_evpd_page_83(scsi);
				res = true;
				break;
			case INQUIRY_VITAL_PAGE_SERIAL_NUM:
				scsi_fill_evpd_page_80(scsi);
				res = true;
				break;
			default:
				break;
			}
	#if (SCSI_DEBUG_FLOW)
			printf("SCSI: inquiry vital data page %02X\n\r", scsi->cmd.address);
	#endif
		}
		//standart inquiry
		else
		{
			scsi_fill_standart_inquiry_page(scsi);
	#if (SCSI_DEBUG_FLOW)
			printf("SCSI: standart inquiry\n\r");
	#endif
			res = true;
		}
		if (!res)
			scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_INVALID_FIELD_IN_CDB);
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

	return res;
}

static inline bool scsi_cmd_mode_select6(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
	{
		switch((scsi->cmd.address >> 8) & 0x3f)
		{
		case 0x1c:
			res = true;
			break;
		default:
			scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_INVALID_FIELD_IN_CDB);
#if (SCSI_DEBUG_UNSUPPORTED)
			printf("SCSI: Mode select: unsupported page 0x%02X\n\r", (scsi->cmd.address >> 8) & 0x3f);
#endif
		}
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	if (res)
		printf("SCSI: Mode select 6 page 0x%02X\n\r", (scsi->cmd.address >> 8) & 0x3f);
#endif
	return res;
}

static inline bool scsi_cmd_mode_select10(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
	{
		switch((scsi->cmd.address >> 24) & 0x3f)
		{
		case 0x1c:
			res = true;
			break;
		default:
			scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_INVALID_FIELD_IN_CDB);
#if (SCSI_DEBUG_UNSUPPORTED)
			printf("SCSI: Mode select: unsupported page 0x%02X\n\r", (scsi->cmd.address >> 8) & 0x3f);
#endif
		}
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	if (res)
		printf("SCSI: Mode select 10 page 0x%02X\n\r", (scsi->cmd.address >> 8) & 0x3f);
#endif
	return res;
}

static inline bool scsi_cmd_mode_sense6(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
	{
		switch((scsi->cmd.address >> 8) & 0x3f)
		{
		case 0x1c:
			scsi_fill_sense_page_1c(scsi);
			res = true;
			break;
		case 0x3f:
			scsi_fill_sense_page_3f(scsi);
			res = true;
			break;
		default:
			scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_INVALID_FIELD_IN_CDB);
#if (SCSI_DEBUG_UNSUPPORTED)
			printf("SCSI: Mode sense: unsupported page 0x%02X\n\r", (scsi->cmd.address >> 8) & 0x3f);
#endif
		}
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	if (res)
		printf("SCSI: Mode sense 6 page 0x%02X\n\r", (scsi->cmd.address >> 8) & 0x3f);
#endif
	return res;
}

static inline bool scsi_cmd_mode_sense10(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_10)
	{
		switch((scsi->cmd.address >> 24) & 0x3f)
		{
		case 0x1c:
			scsi_fill_sense_page_1c(scsi);
			res = true;
			break;
		case 0x3f:
			scsi_fill_sense_page_3f(scsi);
			res = true;
			break;
		default:
			scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_INVALID_FIELD_IN_CDB);
#if (SCSI_DEBUG_UNSUPPORTED)
			printf("SCSI: Mode sense: unsupported page 0x%02X\n\r", (scsi->cmd.address >> 24) & 0x3f);
#endif
		}
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);

#if (SCSI_DEBUG_FLOW)
	if (res)
		printf("SCSI: Mode sense 10, page 0x%02X\n\r", (scsi->cmd.address >> 24) & 0x3f);
#endif
	return res;
}

static inline bool scsi_cmd_test_unit_ready(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
	{
		if (storage_check_media(scsi->storage))
		{
			res = true;
#if (SCSI_DEBUG_FLOW)
			printf("SCSI: unit ready\n\r");
#endif
		}
		else
			scsi_error(scsi, SENSE_KEY_NOT_READY, ASQ_MEDIUM_NOT_PRESENT);
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);
	return res;
}

static inline bool scsi_cmd_start_stop_unit(SCSI* scsi)
{
	bool res = false;
	if (scsi->cmd.cmd_type == SCSI_CMD_6)
	{
		switch (scsi->cmd.len >> 4)
		{
		case 0:
			if (scsi->cmd.len & (1 << 1))
				storage_stop(scsi->storage);
			break;
		case 2:
		case 3:
		case 0xa:
		case 0xb:
			storage_stop(scsi->storage);
			break;
		default:
			break;
		}

		res = true;
#if (SCSI_DEBUG_FLOW)
		printf("SCSI: start/stop unit 0x%1X\n\r", scsi->cmd.len >> 4);
#endif
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);
	return res;
}

bool scsi_cmd(SCSI* scsi, char* cmd_buf, uint8_t cmd_size)
{
	bool res = false;
	if (cmd_size == SCSI_CMD_6 || cmd_size == SCSI_CMD_10 || cmd_size == SCSI_CMD_12 || cmd_size == SCSI_CMD_16)
	{
		scsi_decode_cmd(scsi, cmd_buf, cmd_size);
		switch (scsi->cmd.opcode)
		{
		case SCSI_CMD_REQUEST_SENSE:
			res = scsi_cmd_request_sense(scsi);
			break;
		case SCSI_CMD_INQUIRY:
			res = scsi_cmd_inquiry(scsi);
			break;
		case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
			res = scsi_cmd_prevent_allow_medium_removal(scsi);
			break;
		case SCSI_CMD_TEST_UNIT_READY:
			res = scsi_cmd_test_unit_ready(scsi);
			break;
		case SCSI_CMD_READ6:
			res = scsi_cmd_read6(scsi);
			break;
		case SCSI_CMD_READ10:
			res = scsi_cmd_read10(scsi);
			break;
		case SCSI_CMD_READ12:
			res = scsi_cmd_read12(scsi);
			break;
		case SCSI_CMD_READ16:
			res = scsi_cmd_read16(scsi);
			break;
		case SCSI_CMD_READ_CAPACITY:
			res = scsi_cmd_read_capacity(scsi);
			break;
		case SCSI_CMD_READ_FORMAT_CAPACITY:
			res = scsi_cmd_read_format_capacity(scsi);
			break;
		case SCSI_CMD_SYNCHRONIZE_CACHE:
			res = scsi_cmd_synchronize_cache(scsi);
			break;
		case SCSI_CMD_WRITE6:
			res = scsi_cmd_write6(scsi);
			break;
		case SCSI_CMD_WRITE10:
			res = scsi_cmd_write10(scsi);
			break;
		case SCSI_CMD_WRITE12:
			res = scsi_cmd_write12(scsi);
			break;
		case SCSI_CMD_WRITE16:
			res = scsi_cmd_write16(scsi);
			break;
		case SCSI_CMD_VERIFY6:
			res = scsi_cmd_verify6(scsi);
			break;
		case SCSI_CMD_VERIFY10:
			res = scsi_cmd_verify10(scsi);
			break;
		case SCSI_CMD_VERIFY12:
			res = scsi_cmd_verify12(scsi);
			break;
		case SCSI_CMD_VERIFY16:
			res = scsi_cmd_verify16(scsi);
			break;
		case SCSI_CMD_MODE_SELECT6:
			res = scsi_cmd_mode_select6(scsi);
			break;
		case SCSI_CMD_MODE_SELECT10:
			res = scsi_cmd_mode_select10(scsi);
			break;
		case SCSI_CMD_MODE_SENSE6:
			res = scsi_cmd_mode_sense6(scsi);
			break;
		case SCSI_CMD_MODE_SENSE10:
			res = scsi_cmd_mode_sense10(scsi);
			break;
		case SCSI_CMD_START_STOP_UNIT:
			res = scsi_cmd_start_stop_unit(scsi);
			break;
		default:
			//unsupported opcode:
			scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_INVALID_COMMAND_OPERATION_CODE);
			break;
		}
	}
	else
		scsi_error(scsi, SENSE_KEY_ILLEGAL_REQUEST, ASQ_CDB_DECRYPTION_ERROR);
	return res;
}

SCSI* scsi_create(STORAGE* storage, SCSI_DESCRIPTOR *descriptor)
{
	SCSI* scsi = sys_alloc(sizeof(SCSI));
	if (scsi)
	{
		scsi->storage = storage;
		scsi->descriptor = descriptor;
		scsi->error_head = scsi->error_tail = 0;
	}
	else
		error(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, "SCSI");
	return scsi;
}

void scsi_destroy(SCSI* scsi)
{
	sys_free(scsi);
}

void scsi_reset(SCSI* scsi)
{
	scsi->error_head = scsi->error_tail = 0;
}
