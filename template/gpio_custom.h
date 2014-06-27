/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_CUSTOM_H
#define GPIO_CUSTOM_H

/*
	gpio_custom.h - if defined in arch decoder, this file will be included to override default GPIO/EXTI port/pin names
			below is just sample
 */

typedef enum {
	GPIO_0 = 0,
	GPIO_1,
	GPIO_2,
	GPIO_3,
	GPIO_4,
	GPIO_5,
	GPIO_6,
	GPIO_7,
	GPIO_8,
	GPIO_9,
	GPIO_10,
	GPIO_11,
	GPIO_12,
	GPIO_13,
	GPIO_14,
	GPIO_15,
	GPIO_16,
	GPIO_17,
	GPIO_18,
	GPIO_19,
	GPIO_20,
	GPIO_21,
	GPIO_22,
	GPIO_23,
	GPIO_24,
	GPIO_25,
	GPIO_26,
	GPIO_27,
	GPIO_28,
	GPIO_29,
	GPIO_30,
	GPIO_31,
	GPIO_32,
	GPIO_33,
	GPIO_34,
	GPIO_35,
	GPIO_36,
	GPIO_37,
	GPIO_38,
	GPIO_39,
	GPIO_40,
	GPIO_41,
	GPIO_42,
	GPIO_43,
	GPIO_44,
	GPIO_45,
	GPIO_46,
	GPIO_47,
	GPIO_48,
	GPIO_49,
	GPIO_50,
	GPIO_51,
	GPIO_52,
	GPIO_INVALID
} GPIO_CLASS;

typedef enum {
	EXTI_LINE0 = 0,
	EXTI_LINE1,
	EXTI_LINE2,
	EXTI_LINE3,
	EXTI_LINE4,
	EXTI_LINE5,
	EXTI_LINE6,
	EXTI_LINE7
}EXTI_CLASS;


#endif // GPIO_CUSTOM_H
