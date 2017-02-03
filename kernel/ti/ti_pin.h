/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_PIN_H
#define TI_PIN_H

#include "ti_exo.h"
#include "../../userspace/ti/ti_driver.h"
#include "../../userspace/ipc.h"

typedef struct {
    uint8_t pin_used;
} PIN_DRV;

//for internal use only
void ti_pin_enable(EXO* exo, DIO dio, unsigned int mode, bool direction_out);
void ti_pin_disable(EXO* exo, DIO dio);
void ti_gpio_set_pin(DIO dio);
void ti_gpio_reset_pin(DIO dio);

void ti_pin_init(EXO* exo);
void ti_pin_request(EXO* exo, IPC* ipc);
void ti_pin_gpio_request(EXO* exo, IPC* ipc);

#endif // TI_PIN_H
