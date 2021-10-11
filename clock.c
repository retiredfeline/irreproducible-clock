#define STM8S103

#include "mcu.h"
#include "tick.h"
#include "tod.h"
#include "display.h"
#include "button.h"

#ifdef	DS3231
#include "rtc.h"
#define	TIME	rtc_time
#else
#define	TIME	tod_time
#endif

#include "pt.h"

#define MSPERTICK	2u
#define	DEPMIN		(100u / MSPERTICK)	// debounce period
#define	RPTTHRESH	(400u / MSPERTICK)	// repeat threshold after debounce
#define	RPTPERIOD	(250u / MSPERTICK)	// repeat period

static uint8_t swstate, swtent, swmin, swrepeat, normal_display;
static struct pt pt;
static uint16_t setactive;			// ticks, > 0 in setting mode

#define	SETTIMEOUT	(8*500)			// 8 seconds to set
#define	SETMARGIN	(500/2)			// 0.5 seconds to blank

static void switchaction()
{
	switch (swstate & B_BOTH) {
	case B_MINS:
		if (setactive <= 0)
			break;
		normal_display = 0;		// suspend while setting
		display_set_ptr(D_MINS);
		TIME.seconds = 0;
		TIME.minutes++;
		if (TIME.minutes >= 60)
			TIME.minutes = 0;
		setactive = SETTIMEOUT - SETMARGIN - 1;
		display_update();
		break;
	case B_HOURS:
		if (setactive <= 0)
			break;
		normal_display = 0;		// suspend while setting
		display_set_ptr(D_HOURS);
		TIME.hours++;
		if (TIME.hours >= 24)
			TIME.hours = 0;
		setactive = SETTIMEOUT - SETMARGIN - 1;
		display_update();
		break;
	case B_BOTH:
		setactive = SETTIMEOUT;
		display_set_visibility(0);
		break;
	}
}

static void set_normal_mode(void)
{
	// 3 seconds displaying minutes, then 2 seconds displaying hours
	display_set_ptr((TIME.seconds % 5 < 3) ? D_MINS : D_HOURS);
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

	setactive = 0;					// not in set mode
	uint8_t counter = 0;
	display_update();
	for (;;) {
		if (setactive > 0)			// decrease set timeout
			setactive--;
		display_set_visibility(setactive == 0 ||
			(setactive >= SETMARGIN &&
			setactive < SETTIMEOUT - SETMARGIN));
		if (TIME.changed & (T_HOURS | T_MINUTES))
			display_update();
		if (TIME.changed & T_SECONDS && normal_display)
			set_normal_mode();		// only if not setting
		TIME.changed = T_NONE;
		display_next_digit();
		counter++;
		if (counter >= 250) {
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
