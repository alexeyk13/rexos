/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Pavel P. Morozkin (pavel.morozkin@gmail.com)
*/

#ifndef SX12XX_CONFIG_CHECK_H
#define SX12XX_CONFIG_CHECK_H

//----------------------------- check config correctness -----------------------------
#define VAL LORA_CONTINUOUS_RECEPTION_ON
#if !(VAL == 0 || VAL == 1)
#error invalid params
#endif
#undef VAL

#define VAL LORA_RF_CARRIER_FREQUENCY_MHZ
#if   (LORA_CHIP == SX1272)
#if !(VAL >= 860 && VAL <= 1020)
#error invalid params
#endif
#elif (LORA_CHIP == SX1276)
#if !((VAL >= 862 && VAL <= 1020) || (VAL >= 410 && VAL <= 525) || (VAL >= 137 && VAL <= 175))
#error invalid params
#endif
#elif (LORA_CHIP == SX1261)
#if !((VAL >= 902 && VAL <=  928) || (VAL >= 863 && VAL <= 870) || (VAL >= 779 && VAL <= 787) || \
      (VAL >= 470 && VAL <=  510) || (VAL >= 430 && VAL <= 440))
#error invalid params
#endif
#endif
#undef VAL

#define VAL LORA_SPREADING_FACTOR
#if   (LORA_CHIP == SX1272) || (LORA_CHIP == SX1276)
#if !(VAL >= 6 && VAL <= 12)
#error invalid params
#endif
#elif (LORA_CHIP == SX1261)
#if !(VAL >= 5 && VAL <= 12)
#error invalid params
#endif
#endif
#undef VAL

#define VAL LORA_MAX_PAYLOAD_LEN
#if !(VAL >= 1 && VAL <= 255)
#error invalid params
#endif
#undef VAL

#define VAL LORA_SIGNAL_BANDWIDTH
#if   (LORA_CHIP == SX1272)
#if !(VAL >= 0 && VAL <= 2)
#error invalid params
#endif
#elif (LORA_CHIP == SX1276)
#if !(VAL >= 0 && VAL <= 9)
#error invalid params
#endif
#elif (LORA_CHIP == SX1261)
#if !((VAL >= 0 && VAL <= 6) || (VAL >= 8 && VAL <= 10))
#error invalid params
#endif
#endif
#undef VAL

#define VAL LORA_PA_SELECT
#if !(VAL == 0 || VAL == 1)
#error invalid params
#endif
#undef VAL

#if   (LORA_CHIP == SX1272) || (LORA_CHIP == SX1276)
#define VAL SX127X_ENABLE_HIGH_OUTPUT_POWER
//High Power +20 dBm Operation:
//Notes:
// - High Power settings must be turned off when using PA0
// - The Over Current Protection limit should be adapted to
//   the actual power level, in RegOcp
#if !((LORA_PA_SELECT == 0 &&  VAL == 0) || \
      (LORA_PA_SELECT == 1 && (VAL == 0  || (VAL == 1 && SX127X_OCP_ON == 1))))
#error invalid params
#endif
#undef VAL
#endif

#define VAL LORA_PREAMBLE_LEN
#if   (LORA_CHIP == SX1272) || (LORA_CHIP == SX1276)
#if !(VAL >= 6 && VAL <= 65535)
#error invalid params
#endif
#elif (LORA_CHIP == SX1261)
#if !(VAL >= 10 && VAL <= 65535)
#error invalid params
#endif
#endif
#undef VAL

#if   (LORA_CHIP == SX1272) || (LORA_CHIP == SX1276)
#if (LORA_SPREADING_FACTOR == 6 && !LORA_IMPLICIT_HEADER_MODE_ON)
#error invalid params
#endif
#endif

//add more checks if need

#endif //SX12XX_CONFIG_CHECK_H