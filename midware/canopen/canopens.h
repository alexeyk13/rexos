#ifndef CANOPENS_H
#define CANOPENS_H

#include "sys_config.h"
#include <stdint.h>
#include "object.h"
#include "canopen.h"
#include "can.h"

typedef struct _CO CO;


void co_led_change(CO* co, LED_STATE state);

#endif //CANOPENS_H
