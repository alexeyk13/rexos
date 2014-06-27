#include "gpio_user.h"
#include "gpio.h"
#include "config.h"

void gpio_user_init()
{
	gpio_enable_pin(LED_PIN, PIN_MODE_OUT);
	led_off();
}

void led_on()
{
	gpio_set_pin(LED_PIN, false);
}

void led_off()
{
	gpio_set_pin(LED_PIN, true);
}
