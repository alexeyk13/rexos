/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_RADIO_CONFIG_H_
#define _NRF_RADIO_CONFIG_H_

#include <stdint.h>

// -------------------------------- SETTINGS -----------------------------------
#define RADIO_PROCESS_SIZE                      1024
#define RADIO_PROCESS_PRIORITY                  201

#define RADIO_DEBUG                             1

#if (RADIO_DEBUG)
#define RADIO_DEBUG_INFO                        1
#define RADIO_DEBUG_ERRORS                      0
#define RADIO_DEBUG_REQUESTS                    0
#define RADIO_DEBUG_IO                          0
#endif // RADIO_DEBUG
// -----------------------------------------------------------------------------
// Radio output power settings
#define RADIO_POWER_POS_4dBm                    0x04
#define RADIO_POWER_0dBm                        0x00
#define RADIO_POWER_NEG_4dBm                    0xFC
#define RADIO_POWER_NEG_8dBm                    0xF8
#define RADIO_POWER_NEG_12dBm                   0xF4
#define RADIO_POWER_NEG_16dBm                   0xF0
#define RADIO_POWER_NEG_20dBm                   0xEC
#define RADIO_POWER_NEG_30dBm                   0xD0

#endif /* _NRF_RADIO_CONFIG_H_ */

