#ifndef CANOPENS_PRIVATE_H
#define CANOPENS_PRIVATE_H

#include "sys_config.h"
#include <stdint.h>
#include "object.h"
#include "canopen.h"
#include "can.h"
#include "co_lss.h"
#include "co_def.h"

typedef enum {
    Initialisation = 0x00,
    Disconnected = 0x01,
    Connecting = 0x02,
    Preparing = 0x02,
    Stopped = 0x04,
    Operational = 0x05,
    Pre_operational = 0x7F,
    Unknown_state = 0x0F
} CO_STATE;

typedef enum {
    COT_BUS = 0,
    COT_HEARTBEAT,
    COT_LSS
} CO_TIMER;

//------------------------
typedef struct _CO{
    HANDLE can, device;
    struct {
        HANDLE bus;
        HANDLE heartbeat;
        HANDLE lss;
    } timers;
    CAN_MSG rec_msg, out_msg;
    BUS_STATE bus_state;
    CAN_STATE_t can_state;
//canopen
    uint8_t id;
    CO_STATE co_state;
    LSS_t lss;
} CO;

#endif //CANOPENS_PRIVATE_H
