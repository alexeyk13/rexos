/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_iap.h"
#include "../../userspace/lpc/lpc.h"
#include "../kerror.h"
#include "../../userspace/process.h"

#ifdef LPC18xx

#define IAP_LOCATION (*(volatile unsigned int *)(0x10400100))

#else //LPC11xx

#define IAP_LOCATION 0x1FFF1FF1

#endif //LPC18xx

typedef void (*IAP)(unsigned int command[], unsigned int result[]);

bool lpc_iap(LPC_IAP_TYPE* params, unsigned int cmd)
{
    IAP iap = (IAP)IAP_LOCATION;
    params->req[0] = cmd;
    __disable_irq();
    iap(params->req, params->resp);
    __enable_irq();
    if (params->resp[0] == IAP_RESULT_SUCCESS)
        return true;
    switch (params->resp[0])
    {
    case IAP_RESULT_INVALID_COMMAND:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    case IAP_RESULT_SECTOR_NOT_BLANK:
    case IAP_RESULT_SECTOR_NOT_PREPARED_FOR_WRITE:
        kerror(ERROR_INVALID_STATE);
        break;
    case IAP_RESULT_COMPARE_ERROR:
        kerror(ERROR_CORRUPTED);
        break;
    case IAP_RESULT_BUSY:
        kerror(ERROR_BUSY);
        break;
    case IAP_RESULT_CODE_READ_PROTECTION:
        kerror(ERROR_ACCESS_DENIED);
        break;
    case IAP_RESULT_USER_CODE_CHECKSUM:
        kerror(ERROR_CRC);
        break;
    default:
        kerror(ERROR_INVALID_PARAMS);
    }
    return false;
}
