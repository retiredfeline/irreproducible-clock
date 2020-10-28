#include <stm8s_tim1.h>
#include <stm8s_itc.h>

enum tod_changes {
	T_NONE = 0x0, T_SUBSECOND = 0x1, T_SECONDS = 0x2, T_MINUTES = 0x4, T_HOURS = 0x8,
};

#define	TIM1_RATE	64
#define	HERTZ		32

typedef struct tod_time {
	uint8_t hours, minutes, seconds, subseconds;
	enum tod_changes changed;
};
extern struct tod_time tod_time;

extern void tim1_isr(void) __interrupt(ITC_IRQ_TIM1_OVF);
extern void tod_init(void);
