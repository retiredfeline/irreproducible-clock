#ifdef	DS3231
#include "rtc.h"
#define	TIME	rtc_time
#else
#define	TIME	tod_time
#endif
