/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_RF_PRIVATE_H
#define TI_RF_PRIVATE_H

// RF cmd result byte

#define RF_RESULT_PENDING                       0x00                    /*!< Pending                                                                */
#define RF_RESULT_DONE                          0x01                    /*!< Done                                                                   */

#define RF_RESULT_ILLEGAL_POINTER               0x81                    /*!< Illegal Pointer                                                        */
#define RF_RESULT_UNKNOWN_CMD                   0x82                    /*!< Unknown Command                                                        */
#define RF_RESULT_UNKNOWN_DIR_CMD               0x83                    /*!< Unknown Dir Command                                                    */
#define RF_RESULT_CONTEXT_ERROR                 0x85                    /*!< Context Error                                                          */
#define RF_RESULT_SCHEDULING_ERROR              0x86                    /*!< Scheduling Error                                                       */
#define RF_RESULT_PAR_ERROR                     0x87                    /*!< Par Error                                                              */
#define RF_RESULT_QUEUE_ERROR                   0x88                    /*!< Queue Error                                                            */
#define RF_RESULT_QUEUE_BUSY                    0x89                    /*!< Queue Busy                                                             */

// Protocol-independent Radio Operation Commands

#define RF_CMD_NOP                              0x0801                  /*!< No Operation Command                                                   */
#define RF_CMD_RADIO_SETUP                      0x0802                  /*!< Set Up Radio Settings Command                                          */
#define RF_CMD_FS                               0x0803                  /*!< Frequency Synthesizer Controls Command                                 */
#define RF_CMD_FS_OFF                           0x0804                  /*!< Turn Off Frequency Synthesizer                                         */

#define RF_CMD_RX_TEST                          0x0807                  /*!< Receiver Test Command                                                  */
#define RF_CMD_TX_TEST                          0x0808                  /*!< Transmitter Test Command                                               */
#define RF_CMD_SYNC_STOP_RAT                    0x0809                  /*!< Synchronize and Stop Radio Timer Command                               */
#define RF_CMD_SYNC_START_RAT                   0x080a                  /*!< Synchronously Start Radio Timer Command                                */
#define RF_CMD_COUNT                            0x080b                  /*!< Counter Command                                                        */
#define RF_CMD_FS_POWERUP                       0x080c                  /*!< Power Up Frequency Synthesizer                                         */
#define RF_CMD_FS_POWERDOWN                     0x080d                  /*!< Power Down Frequency Synthesizer                                       */

#define RF_CMD_SCH_IMM                          0x0810                  /*!< Run Immediate Command as Radio Operation                               */

#define RF_CMD_COUNT_BRANCH                     0x0812                  /*!< Counter Command With Branch of Command Chain                           */
#define RF_CMD_PATTERN_CHECK                    0x0813                  /*!< Check a Value in Memory Against a Pattern                              */

// Protocol-Independent Direct and Immediate Commands

//TODO: rest of imm
#define RF_CMD_PING                             0x0406                  /*!<  Respond With Interrupt                                                */

// Immediate Commands for Data Queue Manipulation

//TODO: rest of dq


#endif // TI_RF_PRIVATE_H
