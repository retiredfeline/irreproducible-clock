#include <stm8s_i2c.h>

#include "tchanges.h"

typedef struct rtc_time {
	uint8_t hours, minutes, seconds;
	enum time_changes changed;
};
extern struct rtc_time rtc_time;

extern void rtc_init(void);
extern void rtc_getnow(void);
extern void rtc_update(enum time_changes);
