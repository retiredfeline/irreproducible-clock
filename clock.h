#ifdef	DS3231
#include "rtc.h"
#define	TIME	rtc_time
#else
#include "tod.h"
#define	TIME	tod_time
#endif

enum set_mode { NORMAL_MODE = 0, TIME_HOURS = 1, TIME_MINS = 2, END_MODE = 3 };
extern enum set_mode mode;
