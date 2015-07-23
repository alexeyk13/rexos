/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include "lib.h"

typedef enum {
    GPIO_MODE_OUT = 0,
    GPIO_MODE_IN_FLOAT,
    GPIO_MODE_IN_PULLUP,
    GPIO_MODE_IN_PULLDOWN
} GPIO_MODE;

typedef struct {
    void (*lib_gpio_enable_pin)(unsigned int, GPIO_MODE);
    void (*lib_gpio_enable_mask)(unsigned int, GPIO_MODE, unsigned int);
    void (*lib_gpio_disable_pin)(unsigned int);
    void (*lib_gpio_disable_mask)(unsigned int, unsigned int);
    void (*lib_gpio_set_pin)(unsigned int);
    void (*lib_gpio_set_mask)(unsigned int, unsigned int);
    void (*lib_gpio_reset_pin)(unsigned int);
    void (*lib_gpio_reset_mask)(unsigned, unsigned int);
    bool (*lib_gpio_get_pin)(unsigned int);
    unsigned int (*lib_gpio_get_mask)(unsigned int, unsigned int);
    void (*lib_gpio_set_data_out)(unsigned int, unsigned int);
    void (*lib_gpio_set_data_in)(unsigned int, unsigned int);
} LIB_GPIO;


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
    \brief enable pin mask in GPIO mode
    \param port: port number, hardware specific
    \param mode: pin's mode
    \param mask: pin bit mask, assuming no more than 32 bits in port
    \retval none
*/
void gpio_enable_mask(unsigned int port, GPIO_MODE mode, unsigned int mask);

/**
    \brief disable pin
    \param pin: pin number, hardware specific
    \retval none
*/
void gpio_disable_pin(unsigned int pin);

/**
    \brief disable pin mask
    \param port: port number, hardware specific
    \param mask: pin bit mask, assuming no more than 32 bits in port
    \retval none
*/
void gpio_disable_mask(unsigned int port, unsigned int mask);

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
    \brief set lower bits in port (data bus) out direction
    \param port: port number, hardware specific
    \param wide: number of lower pins, assuming no more than 32 bits in port
    \retval none
*/
void gpio_set_data_out(unsigned int port, unsigned int wide);

/**
    \brief set lower bits in port (data bus) in direction
    \param port: port number, hardware specific
    \param wide: number of lower pins, assuming no more than 32 bits in port
    \retval none
*/
void gpio_set_data_in(unsigned int port, unsigned int wide);

#endif // GPIO_H
