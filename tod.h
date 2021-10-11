#include <stm8s_tim1.h>
#include <stm8s_itc.h>

#include "tchanges.h"

#define	TIM1_RATE	64
#define	HERTZ		32

typedef struct tod_time {
	uint8_t hours, minutes, seconds, subseconds;
	enum time_changes changed;
};
extern struct tod_time tod_time;

extern void tim1_isr(void) __interrupt(ITC_IRQ_TIM1_OVF);
extern void tod_init(void);
