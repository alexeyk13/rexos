/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_exo.h"
#include "lpc_exo_private.h"
#include "lpc_timer.h"
#include "lpc_pin.h"
#include "lpc_gpio.h"
#include "lpc_power.h"
#include "../kernel.h"
#include "../kstdlib.h"
#include "../kerror.h"
#include "../../userspace/object.h"
#include "../../userspace/svc.h"

#include "lpc_uart.h"
#include "lpc_i2c.h"
#ifdef LPC11Uxx
#include "lpc_usb.h"
#else //LPC18xx
#include "lpc_otg.h"
#endif //LPC11Uxx
#include "lpc_eep.h"
#include "lpc_sdmmc.h"
#include "lpc_flash.h"
#include "lpc_eth.h"

void exodriver_delay_us(unsigned int us)
{
    unsigned int i;
    for (i = lpc_power_get_core_clock_inside() / 5000000 * us; i; --i)
        __NOP();
}

void exodriver_post(IPC* ipc)
{
    switch (HAL_GROUP(ipc->cmd))
    {
    case HAL_POWER:
        lpc_power_request(__KERNEL->exo, ipc);
        break;
    case HAL_PIN:
        lpc_pin_request(ipc);
        break;
    case HAL_GPIO:
        lpc_gpio_request(ipc);
        break;
    case HAL_TIMER:
        lpc_timer_request(__KERNEL->exo, ipc);
        break;
#if (LPC_UART_DRIVER)
    case HAL_UART:
        lpc_uart_request(__KERNEL->exo, ipc);
        break;
#endif //LPC_UART_DRIVER
#if (LPC_I2C_DRIVER)
    case HAL_I2C:
        lpc_i2c_request(__KERNEL->exo, ipc);
        break;
#endif //LPC_I2C_DRIVER
#if (LPC_USB_DRIVER)
    case HAL_USB:
#ifdef LPC11Uxx
        lpc_usb_request(__KERNEL->exo, ipc);
#else //LPC18xx
        lpc_otg_request(__KERNEL->exo, ipc);
#endif //LPC11Uxx
        break;
#endif //LPC_USB_DRIVER
#if (LPC_EEPROM_DRIVER)
    case HAL_EEPROM:
        lpc_eep_request(__KERNEL->exo, ipc);
        break;
#endif //LPC_EEPROM_DRIVER
#if (LPC_SDMMC_DRIVER)
    case HAL_SDMMC:
        lpc_sdmmc_request(__KERNEL->exo, ipc);
        break;
#endif //LPC_SDMMC_DRIVER
#if (LPC_FLASH_DRIVER)
    case HAL_FLASH:
        lpc_flash_request(__KERNEL->exo, ipc);
        break;
#endif //LPC_FLASH_DRIVER
#if (LPC_ETH_DRIVER)
    case HAL_ETH:
        lpc_eth_request(__KERNEL->exo, ipc);
        break;
#endif //LPC_ETH_DRIVER
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void exodriver_init()
{
    //ISR disabled at this point
    __KERNEL->exo = kmalloc(sizeof(EXO));

    //manage pools
#if defined(LPC18xx)
    //Local SRAM1
    kstdlib_add_pool(SRAM1_BASE, SRAM1_SIZE);

#ifdef AHB_SRAM_ONE_PIECE
    //AHB SRAM1 + SRAM2/ETB SRAM. Shared with USB
#if (LPC_USB_USE_BOTH)
    kstdlib_add_pool(AHB_SRAM1_BASE, AHB_SRAM1_SIZE + AHB_SRAM2_SIZE - (USB_EP_COUNT_MAX | 1) * 2048);
#else
    kstdlib_add_pool(AHB_SRAM1_BASE, AHB_SRAM1_SIZE + AHB_SRAM2_SIZE - (USB_EP_COUNT_MAX >> 1) * 2048);
#endif //LPC_USB_USE_BOTH
#else
    //AHB SRAM1
    kstdlib_add_pool(AHB_SRAM1_BASE, AHB_SRAM1_SIZE);

    //AHB SRAM2/ETB SRAM. Shared with USB
#if (LPC_USB_USE_BOTH)
    kstdlib_add_pool(AHB_SRAM2_BASE, AHB_SRAM2_SIZE - 2048 * 2);
#else
    kstdlib_add_pool(AHB_SRAM2_BASE, AHB_SRAM2_SIZE - 2048);
#endif //LPC_USB_USE_BOTH
#endif //AHB_SRAM_ONE_PIECE

#endif //LPC18xx

    lpc_power_init(__KERNEL->exo);
    lpc_pin_init(__KERNEL->exo);
    lpc_timer_init(__KERNEL->exo);
#if (LPC_UART_DRIVER)
    lpc_uart_init(__KERNEL->exo);
#endif //LPC_UART_DRIVER
#if (LPC_I2C_DRIVER)
    lpc_i2c_init(__KERNEL->exo);
#endif //LPC_I2C_DRIVER
#if (LPC_SDMMC_DRIVER)
    lpc_sdmmc_init(__KERNEL->exo);
#endif //LPC_SDMMC_DRIVER
#if (LPC_USB_DRIVER)
#ifdef LPC11Uxx
    lpc_usb_init(__KERNEL->exo);
#else //LPC18xx
    lpc_otg_init(__KERNEL->exo);
#endif //LPC11Uxx
#endif //LPC_USB_DRIVER
#if (LPC_EEPROM_DRIVER)
#ifdef LPC18xx
    lpc_eep_init(__KERNEL->exo);
#endif //LPC18xx
#endif //LPC_EEPROM_DRIVER
#if (LPC_FLASH_DRIVER)
    lpc_flash_init(__KERNEL->exo);
#endif //LPC_FLASH_DRIVER
#if (LPC_ETH_DRIVER)
    lpc_eth_init(__KERNEL->exo);
#endif //LPC_ETH_DRIVER
}
