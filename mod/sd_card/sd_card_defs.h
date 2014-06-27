/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SD_CARD_DEFS_H
#define SD_CARD_DEFS_H

#define SD_CARD_SECTOR_SIZE								512

//general sdio commands
#define SDIO_GO_IDLE_STATE									0
#define SDIO_SEND_OP_COND									1
#define SDIO_ALL_SEND_CID									2
#define SDIO_SEND_RELATIVE_ADDR							3
#define SDIO_SET_DSR											4
#define SDIO_SWITCH_FUNC									6
#define SDIO_SELECT_DESELECT_CARD						7
#define SDIO_SEND_IF_COND									8
#define SDIO_SEND_CSD										9
#define SDIO_SEND_CID										10
#define SDIO_VOLTAGE_SWITCH								11
#define SDIO_STOP_TRANSMISSION							12
#define SDIO_SEND_STATUS									13
#define SDIO_GO_INACTIVE_STATE							15
#define SDIO_SET_BLOCKLEN									16
#define SDIO_READ_SINGLE_BLOCK							17
#define SDIO_READ_MULTIPLE_BLOCK							18
#define SDIO_SEND_TUNING_BLOCK							19
#define SDIO_SPEED_CLASS_CONTROL							20
#define SDIO_SET_BLOCK_COUNT								23
#define SDIO_WRITE_SINGLE_BLOCK							24
#define SDIO_WRITE_MULTIPLE_BLOCK						25
#define SDIO_PROGRAM_CSD									27
#define SDIO_SET_WRITE_PROT								28
#define SDIO_CLR_WRITE_PROT								29
#define SDIO_SEND_WRITE_PROT								30
#define SDIO_ERASE_WR_BLK_START_ADDR					32
#define SDIO_ERASE_WR_BLK_END_ADDR						33
#define SDIO_ERASE											38
#define SDIO_LOCK_UNLOCK									42
#define SDIO_APP_CMD											55
#define SDIO_GEN_CMD											56
#define SDIO_READ_OCR										58
#define SDIO_CRC_ON_OFF										59

//application-specific commands
#define SDIO_APP_SET_BUS_WIDTH							6
#define SDIO_APP_SD_STATUS									13
#define SDIO_APP_SET_NUM_WR_BLOCK						22
#define SDIO_APP_SET_WR_BLK_ERASE_COUNT				23
#define SDIO_APP_SD_SEND_OP_COND							41
#define SDIO_APP_SET_CLR_CARD_DETECT					24
#define SDIO_APP_SEND_SCR									51

//voltage range 2.7..3.6v, recommended check pattern
#define SDIO_CHECK_PATTERN									0x000001AA
//voltage range 2.7..3.6v, maximum perfomance
#define SD_VOLTAGE_WINDOW									0x10FF8000

//error bits in R1 response
#define SDIO_R1_ERROR_BITS									0xFFF10008
//due to specification, OUT_OF_RANGE error for last block reading must be ignored
#define SDIO_R1_LAST_ERROR_BITS							0x7FF10008
#define SDIO_R1_BIT_LOCKED									(1ul << 25ul)
#define SDIO_R1_BIT_APP_CMD								(1ul << 5ul)

#define SDIO_R3_INIT_DONE									(1ul << 31ul)
#define SDIO_R3_HCS											(1ul << 30ul)
#define SDIO_R3_CCS											(1ul << 30ul)

#define SDIO_SWITCH_HIGH_SPEED_CHECK					0x00fffff1
#define SDIO_SWITCH_HIGH_SPEED							0x80fffff1
#define SDIO_SWITCH_COMMAND_EC							0x80ffff1f
#define SDIO_SWITCH_STRENGTH_A							0x80fff1ff
#define SDIO_SWITCH_CURRENT_400							0x80ff1fff

typedef enum {
	SDSC_V1,
	SDSC_V2,
	SDHC_V2
} SDIO_CARD_TYPE;

typedef enum {
	SD_CARD_STATE_IDLE = 0,
	SD_CARD_STATE_READY,
	SD_CARD_STATE_IDENT,
	SD_CARD_STATE_STBY,
	SD_CARD_STATE_TRAN,
	SD_CARD_STATE_DATA,
	SD_CARD_STATE_RCV,
	SD_CARD_STATE_PRG,
	SD_CARD_STATE_DIS
} SD_CARD_STATE;

#endif // SD_CARD_DEFS_H
