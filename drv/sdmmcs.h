/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SDMMCS_H
#define SDMMCS_H

#include <stdint.h>
#include <stdbool.h>

#define SDMMC_CMD_GO_IDLE_STATE                                     0
#define SDMMC_CMD_SEND_OP_COND                                      1
#define SDMMC_CMD_ALL_SEND_CID                                      2
#define SDMMC_CMD_SEND_RELATIVE_ADDR                                3
#define SDMMC_CMD_SET_DSR                                           4
#define SDMMC_CMD_SWITCH_FUNC                                       6
#define SDMMC_CMD_SELECT_DESELECT_CARD                              7
#define SDMMC_CMD_SEND_IF_COND                                      8
#define SDMMC_CMD_SEND_CSD                                          9
#define SDMMC_CMD_SEND_CID                                          10
#define SDMMC_CMD_VOLTAGE_SWITCH                                    11
#define SDMMC_CMD_STOP_TRANSMISSION                                 12
#define SDMMC_CMD_SEND_STATUS                                       13
#define SDMMC_CMD_GO_INACTIVE_STATE                                 15
#define SDMMC_CMD_SET_BLOCKLEN                                      16
#define SDMMC_CMD_READ_SINGLE_BLOCK                                 17
#define SDMMC_CMD_READ_MULTIPLE_BLOCK                               18
#define SDMMC_CMD_SEND_TUNING_BLOCK                                 19
#define SDMMC_CMD_SPEED_CLASS_CONTROL                               20
#define SDMMC_CMD_SET_BLOCK_COUNT                                   23
#define SDMMC_CMD_WRITE_SINGLE_BLOCK                                24
#define SDMMC_CMD_WRITE_MULTIPLE_BLOCK                              25
#define SDMMC_CMD_PROGRAM_CSD                                       27
#define SDMMC_CMD_SET_WRITE_PROT                                    28
#define SDMMC_CMD_CLR_WRITE_PROT                                    29
#define SDMMC_CMD_SEND_WRITE_PROT                                   30
#define SDMMC_CMD_ERASE_WR_BLK_START_ADDR                           32
#define SDMMC_CMD_ERASE_WR_BLK_END_ADDR                             33
#define SDMMC_CMD_ERASE                                             38
#define SDMMC_CMD_LOCK_UNLOCK                                       42
#define SDMMC_CMD_APP_CMD                                           55
#define SDMMC_CMD_GEN_CMD                                           56
#define SDMMC_CMD_READ_OCR                                          58
#define SDMMC_CMD_CRC_ON_OFF                                        59

#define SDMMC_R1_AKE_SEQ_ERROR                                      (1 << 3)    /* Error authentication in the sequence process */
#define SDMMC_R1_APP_CMD                                            (1 << 5)    /* The card will expect ACMD, or an indication that the command
                                                                                   has been interpreted as ACMD */
#define SDMMC_R1_READY_FOR_DATA                                     (1 << 8)    /* Corresponds to buffer empty signaling on the bus */

#define SDMMC_R1_CURRENT_STATE_MSK                                  (0xf << 9)  /* The state of the card when receiving the command. If the
                                                                                   command execution causes a state change, it will be
                                                                                   visible to the host in the response to the next command.
                                                                                   The four bits are interpreted as a binary coded number
                                                                                   between 0 and 15 */
#define SDMMC_R1_STATE_IDLE                                         (0x00 << 9)
#define SDMMC_R1_STATE_READY                                        (0x01 << 9)
#define SDMMC_R1_STATE_IDENT                                        (0x02 << 9)
#define SDMMC_R1_STATE_STBY                                         (0x03 << 9)
#define SDMMC_R1_STATE_TRAN                                         (0x04 << 9)
#define SDMMC_R1_STATE_DATA                                         (0x05 << 9)
#define SDMMC_R1_STATE_RCV                                          (0x06 << 9)
#define SDMMC_R1_STATE_PRG                                          (0x07 << 9)
#define SDMMC_R1_STATE_DIS                                          (0x08 << 9)

#define SDMMC_R1_ERASE_RESET                                        (1 << 13)   /* An erase sequence was cleared before executing because an out of
                                                                                   erase sequence command was received */
#define SDMMC_R1_CARD_ECC_DISABLE                                   (1 << 14)   /* The command has been executed without using the internal ECC */
#define SDMMC_R1_WP_ERASE_SKIP                                      (1 << 15)   /* Set when only partial address space was erased due to existing
                                                                                   write protected blocks or the temporary or permanent write protected
                                                                                   card was erased */
#define SDMMC_R1_CSD_OVERWRITE                                      (1 << 16)   /* Can be either one of the following errors:
                                                                                   - The read only section of the CSD does not match the card content
                                                                                   - An attempt to reverse the copy (set as original) or permanent WP
                                                                                     (unprotected) bits was made */
#define SDMMC_R1_ERROR                                              (1 << 19)   /* A general or an unknown error occurred during the operation */
#define SDMMC_R1_CC_ERROR                                           (1 << 20)   /* Internal card controller error */
#define SDMMC_R1_CARD_ECC_FAILED                                    (1 << 21)   /* Card internal ECC was applied but failed to correct the data */
#define SDMMC_R1_ILLEGAL_COMMAND                                    (1 << 22)   /* Command not legal for the card state */
#define SDMMC_R1_COM_CRC_ERROR                                      (1 << 23)   /* The CRC check of the previous command failed */
#define SDMMC_R1_LOCK_UNLOCK_FAILED                                 (1 << 24)   /* Set when a sequence or password error has been detected in
                                                                                   command lock/unlock card command */
#define SDMMC_R1_CARD_IS_LOCKED                                     (1 << 25)   /* When set, signals that the card is locked by the host */
#define SDMMC_R1_WP_VIOLATION                                       (1 << 26)   /* Set when the host attempts to a protected block or to the temporary
                                                                                   or permanent write protected card */
#define SDMMC_R1_ERASE_PARAM                                        (1 << 27)   /* An invalid selection of write-blocks for erase occurred */
#define SDMMC_R1_ERASE_SEQ_ERROR                                    (1 << 28)   /* An error in the sequence of erase commands occured */
#define SDMMC_R1_BLOCK_LEN_ERROR                                    (1 << 29)   /* The transferred block length is not allowed for this card,
                                                                                   or the number of transferred bytes does not match
                                                                                   the block length */
#define SDMMC_R1_ADDRESS_ERROR                                      (1 << 30)   /* A misaligned address which did not match the block length
                                                                                   was used in the command */
#define SDMMC_R1_OUT_OF_RANGE                                       (1 << 31)   /* The command's argument was out of the allowed range for this card*/


//Unrecoverable error
#define SDMMC_R1_FATAL_ERROR                                        (SDMMC_R1_ERROR | SDMMC_R1_CC_ERROR | SDMMC_R1_CARD_ECC_FAILED)
//Invalid state, sequence, arguments
#define SDMMC_R1_APP_ERROR                                          (SDMMC_R1_AKE_SEQ_ERROR | SDMMC_R1_ERASE_RESET | SDMMC_R1_WP_ERASE_SKIP | SDMMC_R1_CSD_OVERWRITE | \
                                                                     SDMMC_R1_ILLEGAL_COMMAND | SDMMC_R1_LOCK_UNLOCK_FAILED | SDMMC_R1_WP_VIOLATION | \
                                                                     SDMMC_R1_ERASE_PARAM | SDMMC_R1_ERASE_SEQ_ERROR | SDMMC_R1_BLOCK_LEN_ERROR | \
                                                                     SDMMC_R1_ADDRESS_ERROR | SDMMC_R1_OUT_OF_RANGE)

#define IF_COND_CHECK_PATTERN                                       0xaa
#define IF_COND_CHECK_PATTERN_MASK                                  0xff
#define IF_COND_VOLTAGE_3_3V                                        (1 << 8)

#define SDMMC_ACMD_SET_BUS_WIDTH                                    6
#define SDMMC_ACMD_SD_STATUS                                        13
#define SDMMC_ACMD_SEND_NUM_WR_BLOCKS                               22
#define SDMMC_ACMD_SET_WR_BLK_ERASE_COUNT                           23
#define SDMMC_ACMD_SD_SEND_OP_COND                                  41
#define SDMMC_ACMD_SET_CLR_CARD_DETECT                              42
#define SDMMC_ACMD_SEND_SCR                                         51

#define OP_COND_VOLTAGE_WINDOW                                      0xff8000
#define OP_COND_S18                                                 (1 << 24)
#define OP_COND_XPC                                                 (1 << 28)
#define OP_COND_HCS                                                 (1 << 30)
#define OP_COND_BUSY                                                (1 << 31)

typedef enum {
    SDMMC_NO_CARD,
    SDMMC_CARD_MMC,
    SDMMC_CARD_SDIO,
    SDMMC_CARD_SDIO_COMBO,
    SDMMC_CARD_SD_V1,
    SDMMC_CARD_SD_V2,
    SDMMC_CARD_HD,
    SDMMC_CARD_XD
} SDMMC_CARD_TYPE;

typedef enum {
    SDMMC_NO_RESPONSE,
    SDMMC_RESPONSE_R1,
    SDMMC_RESPONSE_R1B,
    SDMMC_RESPONSE_R2,
    SDMMC_RESPONSE_R3,
    SDMMC_RESPONSE_R4,
    SDMMC_RESPONSE_R5,
    SDMMC_RESPONSE_R6,
    SDMMC_RESPONSE_R7
} SDMMC_RESPONSE_TYPE;

typedef enum {
    SDMMC_ERROR_OK = 0,
    SDMMC_ERROR_TIMEOUT,
    SDMMC_ERROR_CRC_FAIL,
    SDMMC_ERROR_BUSY,
    SDMMC_ERROR_HARDWARE_FAILURE
} SDMMC_ERROR;

#pragma pack(push, 1)

typedef struct {
    uint8_t crc7;
    uint16_t mdt;
    uint32_t psn;
    uint8_t prv;
    char pnm[5];
    char oid[2];
    uint8_t mid;
} CID;

#pragma pack(pop)

typedef struct {
    SDMMC_CARD_TYPE card_type;
    SDMMC_ERROR last_error;
    void* param;
    uint32_t num_sectors, max_clock;
    uint32_t r1;
    uint32_t serial;
    uint16_t rca;
    uint16_t sector_size;
    bool write_protected, writed_before;
} SDMMCS;


//prototypes, that must be defined by driver
extern void sdmmcs_set_clock(void* param, unsigned int speed);
extern void sdmmcs_set_bus_width(void* param, int width);
extern SDMMC_ERROR sdmmcs_send_cmd(void* param, uint8_t cmd, uint32_t arg, void* resp, SDMMC_RESPONSE_TYPE resp_type);

void sdmmcs_init(SDMMCS* sdmmcs, void* param);
bool sdmmcs_reset(SDMMCS* sdmmcs);
bool sdmmcs_open(SDMMCS* sdmmcs);
//driver is responsable for data transfer configuring prior to cmd execution
bool sdmmcs_read(SDMMCS* sdmmcs, unsigned int block, unsigned int count);
bool sdmmcs_write(SDMMCS* sdmmcs, unsigned int block, unsigned int count);
bool sdmmcs_erase(SDMMCS* sdmmcs, unsigned int block, unsigned int count);
void sdmmcs_stop(SDMMCS* sdmmcs);

#endif // SDMMCS_H
