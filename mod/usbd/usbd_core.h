/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_CORE_H
#define USBD_CORE_H

/*
	processes USB device standart requests
 */

#include "types.h"
#include "dev.h"
#include "usbd.h"

void usbd_inform_state_changed(USBD* usbd);
bool usbd_device_request(USBD* usbd);
bool usbd_interface_request(USBD* usbd);
bool usbd_endpoint_request(USBD* usbd);
//callback, that informs about STATUS stage of SETUP is complete
void usbd_request_completed(USBD* usbd);

#endif // USBD_CORE_H
