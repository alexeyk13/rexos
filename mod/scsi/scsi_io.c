/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "scsi_io.h"
#include "scsi_private.h"
#include <string.h>
#include "kernel_config.h"
#include "dbg.h"
#include "queue.h"

bool scsi_read(SCSI* scsi, unsigned long address, unsigned long sectors_count)
{
	bool res = false;
	switch (storage_read(scsi->storage, address, sectors_count))
	{
	case STORAGE_STATUS_OK:
		res = true;
		break;
	case STORAGE_STATUS_NO_MEDIA:
		scsi_error(scsi, SENSE_KEY_NOT_READY, ASQ_MEDIUM_NOT_PRESENT);
		break;
	case STORAGE_STATUS_HARDWARE_FAILURE:
		scsi_error(scsi, SENSE_KEY_HARDWARE_ERROR, ASQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
		break;
	case STORAGE_STATUS_DATA_PROTECTED:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_WRITE_PROTECTED);
		break;
	case 	STORAGE_STATUS_CRC_ERROR:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_READ_RETRIES_EXHAUSTED);
		break;
	case STORAGE_STATUS_INVALID_ADDRESS:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE);
		break;
	case STORAGE_STATUS_TIMEOUT:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
		break;
	default:
		scsi_error(scsi, SENSE_KEY_ABORTED_COMMAND, ASQ_DATA_PHASE_ERROR);
	}
#if (SCSI_DEBUG_IO_FAIL)
	if (!res)
		printf("SCSI read 0x%08X, %d sector(s) FAIL\n\r", address, sectors_count);
#endif //SCSI_DEBUG_FAIL
	return res;
}

bool scsi_write(SCSI* scsi, unsigned long address, unsigned long sectors_count)
{
	bool res = false;
	switch (storage_write(scsi->storage, address, sectors_count))
	{
	case STORAGE_STATUS_OK:
		res = true;
		break;
	case STORAGE_STATUS_NO_MEDIA:
		scsi_error(scsi, SENSE_KEY_NOT_READY, ASQ_MEDIUM_NOT_PRESENT);
		break;
	case STORAGE_STATUS_HARDWARE_FAILURE:
		scsi_error(scsi, SENSE_KEY_HARDWARE_ERROR, ASQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
		break;
	case STORAGE_STATUS_DATA_PROTECTED:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_WRITE_PROTECTED);
		break;
	case 	STORAGE_STATUS_CRC_ERROR:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_WRITE_ERROR);
		break;
	case STORAGE_STATUS_INVALID_ADDRESS:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE);
		break;
	case STORAGE_STATUS_TIMEOUT:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
		break;
	default:
		scsi_error(scsi, SENSE_KEY_ABORTED_COMMAND, ASQ_DATA_PHASE_ERROR);
	}
#if (SCSI_DEBUG_IO_FAIL)
	if (!res)
		printf("SCSI write 0x%08X, %d sector(s) FAIL\n\r", address, sectors_count);
#endif //SCSI_DEBUG_FAIL
	return res;
}

bool scsi_verify(SCSI* scsi, unsigned long address, unsigned long sectors_count)
{
	bool res = false;
	switch (storage_verify(scsi->storage, address, sectors_count, scsi->cmd.flags & SCSI_VERIFY_BYTCHK))
	{
	case STORAGE_STATUS_OK:
		res = true;
		break;
	case STORAGE_STATUS_NO_MEDIA:
		scsi_error(scsi, SENSE_KEY_NOT_READY, ASQ_MEDIUM_NOT_PRESENT);
		break;
	case STORAGE_STATUS_HARDWARE_FAILURE:
		scsi_error(scsi, SENSE_KEY_HARDWARE_ERROR, ASQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
		break;
	case STORAGE_STATUS_DATA_PROTECTED:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_WRITE_PROTECTED);
		break;
	case 	STORAGE_STATUS_CRC_ERROR:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_READ_RETRIES_EXHAUSTED);
		break;
	case STORAGE_STATUS_MISCOMPARE:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_MISCOMPARE_DURING_VERIFY_OPERATION);
		break;
	case STORAGE_STATUS_INVALID_ADDRESS:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE);
		break;
	case STORAGE_STATUS_TIMEOUT:
		scsi_error(scsi, SENSE_KEY_MEDIUM_ERROR, ASQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
		break;
	default:
		scsi_error(scsi, SENSE_KEY_ABORTED_COMMAND, ASQ_DATA_PHASE_ERROR);
	}
#if (SCSI_DEBUG_IO_FAIL)
	if (!res)
		printf("SCSI verify 0x%08X, %d sector(s) FAIL\n\r", address, sectors_count);
#endif //SCSI_DEBUG_FAIL
	return res;
}
