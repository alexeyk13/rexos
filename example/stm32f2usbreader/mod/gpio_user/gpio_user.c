#include "gpio_user.h"
#include "gpio.h"
#include "config.h"

const GPIO_CLASS LED_PINS[]			= {LED1_PIN, LED2_PIN};

void gpio_user_init()
{
	int i;
	for (i = 0; i < LED_MAX; ++i)
	{
		gpio_enable_pin(LED_PINS[i], PIN_MODE_OUT);
		led_off(i);
	}
}

void gpio_user_disable()
{
	int i;
	for (i = 0; i < LED_MAX; ++i)
	{
		led_off(i);
		gpio_disable_pin(LED_PINS[i]);
	}
}

void led_on(LEDS led)
{
	gpio_set_pin(LED_PINS[led], false);
}

void led_off(LEDS led)
{
	gpio_set_pin(LED_PINS[led], true);
}
