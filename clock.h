#ifdef	DS3231
#include "rtc.h"
#define	TIME	rtc_time
#else
#include "tod.h"
#define	TIME	tod_time
#endif
