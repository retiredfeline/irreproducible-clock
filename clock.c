#define STM8S103

#include "clock.h"

#include "mcu.h"
#include "tick.h"
#include "display.h"
#include "button.h"

#include "pt.h"

#define MSPERTICK	2u
#define	DEPMIN		(100u / MSPERTICK)	// debounce period
#define	RPTTHRESH	(400u / MSPERTICK)	// repeat threshold after debounce
#define	RPTPERIOD	(250u / MSPERTICK)	// repeat period
#define	BUTTON_TIMEOUT	64u			// revert to NORMAL_MODE if no button press

enum set_mode mode;
static uint8_t swstate, swtent, swmin, swrepeat, button_timeout;
static struct pt pt;

#define	SETTIMEOUT	(8*500)			// 8 seconds to set
#define	SETMARGIN	(500/2)			// 0.5 seconds to blank

static void switchaction()
{
	switch (swstate & B_BOTH) {
	case B_0:
		mode++;
		switch (mode) {
		case TIME_HOURS:
			display_set_ptr(D_HOURS);
			break;
		case TIME_MINS:
			display_set_ptr(D_MINS);
			break;
		default:
			mode = NORMAL_MODE;
		}
		break;
	case B_1:
		switch (mode) {
		case NORMAL_MODE:
			break;
		case TIME_HOURS:
			TIME.hours++;
			if (TIME.hours >= 24)
				TIME.hours = 0;
#ifdef	DS3231
			rtc_update(T_HOURS);
#endif
			break;
		case TIME_MINS:
			TIME.seconds = 0;
			TIME.minutes++;
			if (TIME.minutes >= 60)
				TIME.minutes = 0;
#ifdef	DS3231
			rtc_update(T_MINUTES);
#endif
			break;
		}
		display_update();
		break;
	}
	if (mode == NORMAL_MODE)
		button_timeout = 0;
	else
		button_timeout = BUTTON_TIMEOUT;
}

static inline void reinitstate()
{
	swmin = DEPMIN;
	swrepeat = RPTTHRESH;
	swtent = swstate = B_NONE;
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
#ifdef	DS3231
	rtc_init();
#else
	tod_init();
#endif
	display_init();
	reinitstate();
	button_init();
	mcu_enable_interrupts();

	uint8_t counter = 0;
#ifdef	DS3231
	rtc_getnow();
#endif
	mode = NORMAL_MODE;
	button_timeout = 0;
	for (;;) {
		if (TIME.changed & (T_HOURS | T_MINUTES | T_SECONDS)) {
			display_update();
			if (button_timeout == 0)	// if no activity, revert to NORMAL_MODE
				mode = NORMAL_MODE;
			else
				button_timeout--;
		}
		TIME.changed = T_NONE;
		display_next_digit();
		counter++;
		if (counter >= 125) {
			counter = 0;
#ifdef	DS3231
			rtc_getnow();
			display_update();
#endif
		}
		while (!tick_check())
			;
		if (display_digit_number() == 0u)
			while(!tick_check())	// give RED "digit" 2x time
				;
		swstate = button_state();
		PT_SCHEDULE(switchhandler(&pt));
		if (mode == NORMAL_MODE) {
			// 3 seconds displaying minutes, then 2 seconds displaying hours
			display_set_ptr((TIME.seconds % 5 < 3) ? D_MINS : D_HOURS);
		}
	}
}
