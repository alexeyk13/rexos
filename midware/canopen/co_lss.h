#ifndef LSS_H
#define LSS_H

#define LSS_DELAY_MS     10

#include "canopens.h"
#include "io.h"

//------------ LSS --------
typedef enum {
// slave states
    LSS_WAITING = 0,
    LSS_CONFIGURATION,
// master states
    LSS_RESET,
    LSS_BIT_CHECK,
    LSS_TEST,
    LSS_SET_ID
} LSS_STATE;

typedef struct {
    LSS_FIND req;
    CO_IDENTITY lss_id;
    LSS_STATE state;
    uint32_t curr_pos;// 0 - vendor, ... 3 - serial
    uint32_t bit_checked;
    bool response;
//    uint32_t remote_id;
} LSS_t;


void lss_request(CO* co);
void lss_timeout(CO* co);
void lss_init_find(CO* co, IO* io);
void lss_init_set_id(CO* co, uint32_t id);

#endif //LSS_H
