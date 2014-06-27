#include "gpio_user.h"
#include "config.h"
#include "dbg.h"
#include "thread.h"
#include "sys_time.h"

void test_thread1(void* param)
{
	printf("test thread1 started\n\r");
	TIME uptime;
	for (;;)
	{
		get_uptime(&uptime);
		printf("uptime: %02d:%02d.%03d\n\r", uptime.sec / 60, uptime.sec % 60, uptime.usec / 1000);
		sleep_ms(1000);
	}
}

void test_thread2(void* param)
{
	printf("test thread2 started\n\r");
	bool on = false;
	for (;;)
	{
		on ? led_on() : led_off();
		on = !on;
		sleep_ms(500);
	}
}

void application_init(void)
{
	gpio_user_init();

	thread_create_and_run("TEST1", 64, 20, test_thread1, NULL);
	thread_create_and_run("TEST2", 64, 30, test_thread2, NULL);
}

void idle_task(void)
{
	for (;;)	{}
}
