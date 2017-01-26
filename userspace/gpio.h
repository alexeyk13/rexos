/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include "ipc.h"

typedef enum {
    GPIO_MODE_OUT = 0,
    GPIO_MODE_IN_FLOAT,
    GPIO_MODE_IN_PULLUP,
    GPIO_MODE_IN_PULLDOWN
} GPIO_MODE;

typedef enum {
    IPC_GPIO_ENABLE_PIN = IPC_USER,
    //warning: deprecated
    IPC_GPIO_ENABLE_MASK,
    IPC_GPIO_DISABLE_PIN,
    //warning: deprecated
    IPC_GPIO_DISABLE_MASK,
    IPC_GPIO_SET_PIN,
    IPC_GPIO_SET_MASK,
    IPC_GPIO_RESET_PIN,
    IPC_GPIO_RESET_MASK,
    IPC_GPIO_GET_PIN,
    IPC_GPIO_GET_MASK,
    IPC_GPIO_SET_DATA_OUT,
    IPC_GPIO_SET_DATA_IN,
    IPC_GPIO_MAX
} GPIO_IPCS;

/** \addtogroup GPIO GPIO
    general purpose IO

    \{
 */

/**
    \brief enable pin in GPIO mode
    \param pin: pin number, hardware specific
    \param mode: pin mode
    \retval none
*/
void gpio_enable_pin(unsigned int pin, GPIO_MODE mode);

/**
    \brief disable pin
    \param pin: pin number, hardware specific
    \retval none
*/
void gpio_disable_pin(unsigned int pin);

/**
    \brief set out pin
    \param pin: pin number, hardware specific
    \retval none
*/
void gpio_set_pin(unsigned int pin);

/**
    \brief set out port by mask
    \param port: port number, hardware specific
    \param mask: pin bit mask, assuming no more than 32 bits in port
    \retval none
*/
void gpio_set_mask(unsigned int port, unsigned int mask);

/**
    \brief reset out pin
    \param pin: pin number, hardware specific
    \retval none
*/
void gpio_reset_pin(unsigned int pin);

/**
    \brief reset out port by mask
    \param port: port number, hardware specific
    \param mask: pin bit mask, assuming no more than 32 bits in port
    \retval none
*/
void gpio_reset_mask(unsigned int port, unsigned int mask);

/**
    \brief get pin value
    \param pin: port number, hardware specific
    \retval true if set
*/
bool gpio_get_pin(unsigned int pin);

/**
    \brief get port pins value by mask
    \param port: port number, hardware specific
    \param mask: pin bit mask, assuming no more than 32 bits in port
    \retval masked value
*/
unsigned int gpio_get_mask(unsigned int port, unsigned int mask);

/**
    \brief set bits in port by mask out direction
    \param port: port number, hardware specific
    \param mask: pin bit mask, assuming no more than 32 bits in port
    \retval none
*/
void gpio_set_data_out(unsigned int port, unsigned int mask);

/**
    \brief set bits in port by mask in direction
    \param port: port number, hardware specific
    \param mask: pin bit mask, assuming no more than 32 bits in port
    \retval none
*/
void gpio_set_data_in(unsigned int port, unsigned int mask);

/** \} */ // end of gpio group

#endif // GPIO_H
