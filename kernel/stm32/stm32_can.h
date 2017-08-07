/*
 RExOS - embedded RTOS
 Copyright (c) 2011-2017, Alexey Kramarenko
 All rights reserved.
 */

#ifndef STM32_CAN_H
#define STM32_CAN_H

/*
 STM32 CAN driver
 */

#include "../../userspace/process.h"
#include "../../userspace/io.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../../userspace/can.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "stm32_exo.h"
#include <stdbool.h>

#define MAX_TX_ERR_CNT     100
#define TX_QUEUE_LEN       10

typedef struct {
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
}CAN_TX_ENTRY;

typedef struct {
    uint32_t head;
    uint32_t tail;
    CAN_TX_ENTRY entry[TX_QUEUE_LEN];
}CAN_TX_QUEUE;

typedef struct {
    HANDLE device;
    HANDLE timer;
    CAN_ERROR error;
    CAN_STATE state;
    uint32_t tx_err_cnt;
    uint32_t max_tx_err;
    CAN_TX_QUEUE queue;
} CAN_DRV;

void stm32_can_init(EXO* exo);
void stm32_can_request(EXO* exo, IPC* ipc);

#endif // STM32_CAN_H
