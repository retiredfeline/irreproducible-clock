#define STM8S103

#include <stm8s_tim1.h>
#include <stm8s_itc.h>

#include "tod.h"

#define	DDS_BITS	24ul
#define	DDS_CARRY	(1ul << DDS_BITS)
#define	DDS_MASK	(DDS_CARRY - 1ul)
#define	DDS_INCR	((DDS_CARRY * HERTZ)/TIM1_RATE)

// Can be adjusted in Makefile, -ve to slow, +ve to speed
#ifndef	DDS_ADJ
#define	DDS_ADJ		0ul
#endif	// DDS_ADJ

static uint32_t DDS_PhaseIncr;
static volatile uint32_t DDS_Accum;

struct tod_time tod_time;

static void tod_increment(void)
{
	DDS_Accum += DDS_PhaseIncr;
	if (!(DDS_Accum & DDS_CARRY))
		return;
	DDS_Accum &= DDS_MASK;
	tod_time.subseconds++;
	tod_time.changed |= T_SUBSECOND;
	if (tod_time.subseconds < HERTZ)
		return;
	tod_time.subseconds = 0;
	tod_time.seconds++;
	tod_time.changed |= T_SECONDS;
	if (tod_time.seconds < 60)
		return;
	tod_time.seconds = 0;
	tod_time.minutes++;
	tod_time.changed |= T_MINUTES;
	if (tod_time.minutes < 60)
		return;
	tod_time.minutes = 0;
	tod_time.hours++;
	tod_time.changed |= T_HOURS;
	if (tod_time.hours < 24)
		return;
	tod_time.hours = 0;
}

void tim1_isr(void) __interrupt(ITC_IRQ_TIM1_OVF)
{
	tod_increment();
	TIM1_ClearITPendingBit(TIM1_IT_UPDATE);	// clear interrupt
}

void tod_init(void)
{
	DDS_PhaseIncr = DDS_INCR + DDS_ADJ;
	tod_time.hours = 12;
	tod_time.minutes = 34;
	tod_time.seconds = 56;
	tod_time.subseconds = 0;
	tod_time.changed = T_NONE;
	TIM1_DeInit();
	// (clock CPU/50000)/5 -> 64 (TIM1_RATE)
	TIM1_TimeBaseInit(50000, TIM1_COUNTERMODE_DOWN, 4, 0);
	TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
	TIM1_Cmd(ENABLE);
}
