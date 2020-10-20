#define STM8S103

#include "mcu.h"
#include "tick.h"
#include "tod.h"
#include "display.h"
#include "button.h"

#include "pt.h"

#define MSPERTICK	2u
#define	DEPMIN		(100u / MSPERTICK)	// debounce period
#define	RPTTHRESH	(400u / MSPERTICK)	// repeat threshold after debounce
#define	RPTPERIOD	(250u / MSPERTICK)	// repeat period

static uint8_t swstate, swtent, swmin, swrepeat, normal_display;
static struct pt pt;

static void switchaction()
{
	switch (swstate & B_BOTH) {
	case B_MINS:
		normal_display = 0;		// suspend while setting
		display_set_ptr(D_MINS);
		tod_time.seconds = 0;
		tod_time.minutes++;
		if (tod_time.minutes >= 60)
			tod_time.minutes = 0;
		display_update();
		break;
	case B_HOURS:
		normal_display = 0;		// suspend while setting
		display_set_ptr(D_HOURS);
		tod_time.hours++;
		if (tod_time.hours >= 24)
			tod_time.hours = 0;
		display_update();
		break;
	}
}

static void set_normal_mode(void)
{
	// 3 seconds displaying minutes, then 2 seconds displaying hours
	display_set_ptr((tod_time.seconds % 5 <= 3) ? D_MINS : D_HOURS);
}

static inline void reinitstate()
{
	swmin = DEPMIN;
	swrepeat = RPTTHRESH;
	swtent = swstate = B_NONE;
	normal_display = 1;
	set_normal_mode();
}

static
PT_THREAD(switchhandler(struct pt *pt))
{
	PT_BEGIN(pt);
	PT_WAIT_UNTIL(pt, swstate != swtent);
	swtent = swstate;
	PT_WAIT_UNTIL(pt, --swmin <= 0 || swstate != swtent);
	if (swstate != swtent) {		// changed, restart
		reinitstate();
		PT_RESTART(pt);
	}
	switchaction();
	PT_WAIT_UNTIL(pt, --swrepeat <= 0 || swstate != swtent);
	if (swstate != swtent) {		// changed, restart
		reinitstate();
		PT_RESTART(pt);
	}
	switchaction();
	for (;;) {
		swrepeat = RPTPERIOD;
		PT_WAIT_UNTIL(pt, --swrepeat <= 0 || swstate == B_NONE);
		if (swstate == B_NONE) {	// released, restart
			reinitstate();
			PT_RESTART(pt);
		}
		switchaction();
	}
	PT_END(pt);
}

int main(void)
{
	mcu_init();
	tick_init();
	tod_init();
	display_init();
	reinitstate();
	button_init();
	mcu_enable_interrupts();

	uint8_t counter = 0;
	display_update();
	for (;;) {
		if (tod_time.changed & (T_HOURS | T_MINUTES))
			display_update();
		if (tod_time.changed & T_SECONDS && normal_display)
			set_normal_mode();		// only if not setting
		tod_time.changed = T_NONE;
		display_next_digit();
		counter++;
		if (counter >= 250) {
			// display_update_dot();	// doesn't look good
			// display_flip_b5();		// debugging
			counter = 0;
		}
		while (!tick_check())
			;
		if (display_digit_number() == 0u)
			while(!tick_check())	// give RED "digit" 2x time
				;
		swstate = button_state();
		PT_SCHEDULE(switchhandler(&pt));
	}
}
