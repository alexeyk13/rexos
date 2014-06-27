#ifndef GPIO_USER_H
#define GPIO_USER_H

typedef enum {
	LED_1 = 0,
	LED_2,
	LED_MAX
} LEDS;


void gpio_user_init();
void gpio_user_disable();
void led_on(LEDS led);
void led_off(LEDS led);

#endif // GPIO_USER_H
