#ifndef LIB_TIME_H
#define LIB_TIME_H

#include "../userspace/lib/types.h"

time_t __mktime(struct tm* ts);
struct tm* __gmtime(time_t time, struct tm* ts);
int __time_compare(TIME* from, TIME* to);
void __time_add(TIME* from, TIME* to, TIME* res);
void __time_sub(TIME* from, TIME* to, TIME* res);
void __us_to_time(int us, TIME* time);
void __ms_to_time(int ms, TIME* time);
int __time_to_us(TIME* time);
int __time_to_ms(TIME* time);
TIME* __time_elapsed(TIME* from, TIME* res);
unsigned int __time_elapsed_ms(TIME* from);
unsigned int __time_elapsed_us(TIME* from);

#endif // LIB_TIME_H
