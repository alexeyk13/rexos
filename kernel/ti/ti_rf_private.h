/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_RF_PRIVATE_H
#define TI_RF_PRIVATE_H

#include <stdint.h>

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

// Radio protocol status codes

#define RF_STATUS_IDLE                          0x0000                  /*!< Operation has not started                                              */
#define RF_STATUS_PENDING                       0x0001                  /*!< Waiting for start trigger                                              */
#define RF_STATUS_ACTIVE                        0x0002                  /*!< Running operation                                                      */
#define RF_STATUS_SKIPPED                       0x0003                  /*!< Operation skipped due to condition in another command                  */

#define RF_STATUS_DONE_OK                       0x0400                  /*!< Operation ended normally                                               */
#define RF_STATUS_DONE_COUNTDOWN                0x0401                  /*!< Counter reached zero                                                   */
#define RF_STATUS_DONE_RXERR                    0x0402                  /*!< Operation ended with CRC error                                         */
#define RF_STATUS_DONE_TIMEOUT                  0x0403                  /*!< Operation ended with time-out                                          */
#define RF_STATUS_DONE_STOPPED                  0x0404                  /*!< Operation stopped after CMD_STOP command                               */
#define RF_STATUS_DONE_ABORT                    0x0405                  /*!< Operation aborted by CMD_ABORT command                                 */

#define RF_STATUS_ERROR_PAST_START              0x0800                  /*!< The start trigger occurred in the past                                 */
#define RF_STATUS_ERROR_START_TRIG              0x0801                  /*!< Illegal start trigger parameter                                        */
#define RF_STATUS_ERROR_CONDITION               0x0802                  /*!< Illegal condition for next operation                                   */
#define RF_STATUS_ERROR_PAR                     0x0803                  /*!< Error in a command specific parameter                                  */
#define RF_STATUS_ERROR_POINTER                 0x0804                  /*!< Invalid pointer to next operation                                      */
#define RF_STATUS_ERROR_CMDID                   0x0805                  /*!< Next operation has a command ID that is undefined or not a
                                                                             radio operation command                                                */

#define RF_STATUS_ERROR_NO_SETUP                0x0807                  /*!< Operation using RX, TX or synthesizer attempted without
                                                                             CMD_RADIO_SETUP                                                        */
#define RF_STATUS_ERROR_NO_FS                   0x0808                  /*!< Operation using RX or TX attempted without the synthesizer being
                                                                             programmed or powered on                                               */
#define RF_STATUS_ERROR_SYNTH_PROG              0x0809                  /*!< Synthesizer programming failed                                         */
#define RF_STATUS_ERROR_TXUNF                   0x080a                  /*!< Modem TX underflow observed                                            */
#define RF_STATUS_ERROR_RXOVF                   0x080b                  /*!< Modem RX overflow observed                                             */
#define RF_STATUS_ERROR_NO_RX                   0x080c                  /*!< Data requested from last RX when no such data exists                   */

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

//for FSM only
#define RF_CMD_IDLE                             0x0000

#define RF_CMD_UPDATE_RADIO_SETUP               0x0001                  /*!< Update Radio Settings Command                                          */
#define RF_CMD_GET_FW_INFO                      0x0002                  /*!< Request Information on the Firmware Being Run                          */

#define RF_CMD_SET_RAT_CMP                      0x000a                  /*!< Set RAT Channel to Compare Mode                                        */

#define RF_CMD_SET_TX_POWER                     0x0010                  /*!< Set Transmit Power                                                     */
#define RF_CMD_UPDATE_FS                        0x0011                  /*!< Set New Synthesizer Frequency Without Recalibration                    */

#define RF_CMD_ABORT                            0x0401                  /*!< ABORT Command                                                          */
#define RF_CMD_STOP                             0x0402                  /*!< Stop Command                                                           */
#define RF_CMD_GET_RSSI                         0x0403                  /*!< Read RSSI Command                                                      */
#define RF_CMD_TRIGGER                          0x0404                  /*!< Generate Command Trigger                                               */
#define RF_CMD_START_RAT                        0x0405                  /*!< Asynchronously Start Radio Timer Command                               */
#define RF_CMD_PING                             0x0406                  /*!< Respond With Interrupt                                                 */

#define RF_CMD_DISABLE_RAT_CH                   0x0408                  /*!< Disable RAT Channel                                                    */
#define RF_CMD_ARM_RAT_CH                       0x0409                  /*!< Arm RAT Channel                                                        */
#define RF_CMD_DISARM_RAT_CH                    0x040a                  /*!< Disarm RAT Channel                                                     */

#define RF_CMD_BUS_REQUES                       0x040e                  /*!< Request System BUS Available for RF Core                               */

#define RF_CMD_READ_RFREG                       0x0601                  /*!< Read RF Core Register                                                  */
#define RF_CMD_SET_RAT_CPT                      0x0603                  /*!< Set RAT Channel to Capture Mode                                        */
#define RF_CMD_SET_RAT_OUTPUT                   0x0604                  /*!< Set RAT Output to a Specified Mode                                     */


// Immediate Commands for Data Queue Manipulation

#define RF_CMD_ADD_DATA_ENTRY                   0x0005                  /*!< Add Data Entry to Queue                                                */
#define RF_CMD_REMOVE_DATA_ENTRY                0x0006                  /*!< Remove First Data Entry From Queue                                     */
#define RF_CMD_FLUSH_QUEUE                      0x0007                  /*!< Flush Queue                                                            */
#define RF_CMD_CLEAR_RX                         0x0008                  /*!< Clear All RX Queue Entries                                             */
#define RF_CMD_REMOVE_PENDING_ENTRIES           0x0009                  /*!< Remove Pending Entries From Queue                                      */


// Bluetooth low energy Radio Operation Commands

#define RF_CMD_BLE_SLAVE                        0x1801                  /*!< Start slave operation                                                  */
#define CMD_BLE_MASTER                          0x1802                  /*!< Start master operation                                                 */
#define CMD_BLE_ADV                             0x1803                  /*!< Start connectable undirected advertiser operation                      */
#define CMD_BLE_ADV_DIR                         0x1804                  /*!< Start connectable directed advertiser operation                        */
#define CMD_BLE_ADV_NC                          0x1805                  /*!< Start the not-connectable advertiser operation                         */
#define CMD_BLE_ADV_SCAN                        0x1806                  /*!< Start scannable undirected advertiser operation                        */
#define CMD_BLE_SCANNER                         0x1807                  /*!< Start scanner operation                                                */
#define CMD_BLE_INITIATOR                       0x1808                  /*!< Start initiator operation                                              */
#define CMD_BLE_GENERIC_RX                      0x1809                  /*!< Receive generic packets (used for PHY test or packet sniffing)         */
#define CMD_BLE_TX_TEST                         0x180a                  /*!< Transmit PHY test packets                                              */


// Bluetooth low-energy-specific immediate commands

#define CMD_BLE_ADV_PAYLOAD                     0x1001                  /*!< Modify payload used in advertiser operations                           */


// Radio operation structure

#pragma pack(push, 1)

typedef struct _RF_CMD_HEADER_TYPE{
    uint16_t commandNo;                                                 /*!< The command ID number                                                  */
    uint16_t status;                                                    /*!< An integer telling the status of the command                           */
    void* pNextOp;                                                      /*!< Pointer to the next operation to run after this operation is done      */
    uint32_t startTime;                                                 /*!< Absolute or relative start time                                        */
    uint8_t startTrigger;                                               /*!< Identification of the trigger that starts the operation                */
    uint8_t condition;                                                  /*!< Condition for running next operation                                   */
} RF_CMD_HEADER_TYPE;

#define RF_CMD_HEADER_TRIG_TYPE_NOW             0
#define RF_CMD_HEADER_TRIG_TYPE_NEVER           1
#define RF_CMD_HEADER_TRIG_TYPE_ABSTIME         2
#define RF_CMD_HEADER_TRIG_TYPE_REL_SUBMIT      3
#define RF_CMD_HEADER_TRIG_TYPE_REL_START       4
#define RF_CMD_HEADER_TRIG_TYPE_REL_PREVSTART   5
#define RF_CMD_HEADER_TRIG_TYPE_REL_FIRSTSTART  6
#define RF_CMD_HEADER_TRIG_TYPE_REL_PREVEND     7
#define RF_CMD_HEADER_TRIG_TYPE_REL_EVT1        8
#define RF_CMD_HEADER_TRIG_TYPE_REL_EVT2        9
#define RF_CMD_HEADER_TRIG_TYPE_EXTERNAL        10

#define RF_CMD_HEADER_TRIG_B_ENA                (1 << 4)
#define RF_CMD_HEADER_TRIG_NO(no)               (((no) & 3) << 5)
#define RF_CMD_HEADER_TRIG_PAST                 (1 << 7)

#define RF_CMD_HEADER_COND_RULE_ALWAYS          0
#define RF_CMD_HEADER_COND_RULE_NEVER           1
#define RF_CMD_HEADER_COND_RULE_STOP_ON_FALSE   2
#define RF_CMD_HEADER_COND_RULE_STOP_ON_TRUE    3
#define RF_CMD_HEADER_COND_RULE_SKIP_ON_FALSE   4
#define RF_CMD_HEADER_COND_RULE_SKIP_ON_TRUE    5
#define RF_CMD_HEADER_COND_SKIP(n)              (((n) & 0xf) << 4)


typedef struct {
    RF_CMD_HEADER_TYPE header;
    uint8_t mode;                                                       /*!< This is the main mode to use                                           */
    uint8_t loDivider;                                                  /*!< Reserved                                                               */
    uint16_t config;
    uint16_t txPower;                                                   /*!< Output power setting; use value from SmartRF Studio                    */
    const void* pRegOverride;                                           /*!< Pointer to a list of hardware and configurationregisters to override.
                                                                             If NULL, no override is used                                           */
} RF_CMD_RADIO_SETUP_TYPE;

#define RF_CMD_RADIO_SETUP_MODE_BLE                                     0x00
#define RF_CMD_RADIO_SETUP_MODE_IEEE_802_15_4                           0x01
#define RF_CMD_RADIO_SETUP_MODE_2MBPS_GFSK                              0x02
#define RF_CMD_RADIO_SETUP_MODE_5MBPS_CODED_8FSK                        0x05
#define RF_CMD_RADIO_SETUP_MODE_KEEP                                    0xff

#define RF_CMD_RADIO_SETUP_CONFIG_MODE_DIFFERENTIAL                     0x0
#define RF_CMD_RADIO_SETUP_CONFIG_MODE_SINGLE_ENDED_RFP                 0x1
#define RF_CMD_RADIO_SETUP_CONFIG_MODE_SINGLE_ENDED_RFN                 0x2
#define RF_CMD_RADIO_SETUP_CONFIG_MODE_ANTHENNA_DIVERSITY_RFP           0x3
#define RF_CMD_RADIO_SETUP_CONFIG_MODE_ANTHENNA_DIVERSITY_RFN           0x4
#define RF_CMD_RADIO_SETUP_CONFIG_MODE_SINGLE_ENDED_RFP_EXTERNAL        0x5
#define RF_CMD_RADIO_SETUP_CONFIG_MODE_SINGLE_ENDED_RFN_EXTERNAL        0x6

#define RF_CMD_RADIO_SETUP_CONFIG_BIAS_INTERNAL                         (0x0ul << 3)
#define RF_CMD_RADIO_SETUP_CONFIG_BIAS_EXTERNAL                         (0x1ul << 3)

#define RF_CMD_RADIO_SETUP_CONFIG_NO_FS_POWERUP                         (0x1ul << 10)

typedef struct {
    RF_CMD_HEADER_TYPE header;
    uint16_t counter;                                                   /*!< Counter. When starting, the radio CPU decrements the value, and
                                                                             the end status of the operation differs if the result is zero.         */
} RF_CMD_COUNT_TYPE;

typedef struct {
    uint16_t commandNo;                                                 /*!< The command ID number                                                  */
} RF_CMD_IMMEDIATE_GENERIC_TYPE;

typedef struct {
    uint16_t commandNo;                                                 /*!< The command ID number                                                  */
    uint16_t versionNo;                                                 /*!< Firmware version number                                                */
    uint16_t startOffset;                                               /*!< The start of free RAM                                                  */
    uint16_t freeRamSz;                                                 /*!< The size of free RAM                                                   */
    uint16_t availRatCh;                                                /*!< Bitmap of available RAT channels                                       */
} RF_CMD_GET_FW_INFO_TYPE;

typedef struct {
    uint16_t commandNo;                                                 /*!< The command ID number                                                  */
    uint16_t txPower;                                                   /*!< Output power setting; use value from SmartRF Studio                    */
} RF_CMD_SET_TX_POWER_TYPE;

#pragma pack(pop)

#endif // TI_RF_PRIVATE_H
