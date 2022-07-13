#define STM8S103

#include <stm8s_tim4.h>
#include <stm8s_itc.h>

#include "tick.h"

static uint8_t volatile tim4_overflow;

void tim4_isr(void) __interrupt(ITC_IRQ_TIM4_OVF)
{
	tim4_overflow = 1;				// set flag
	TIM4_ClearITPendingBit(TIM4_IT_UPDATE);		// clear interrupt
}

void tick_init(void)
{
	tim4_overflow = 0;
	TIM4_DeInit();
	TIM4_TimeBaseInit(TIM4_PRESCALER_128, 249);	// freq = 500Hz
	TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);		// update on overflow
	TIM4_Cmd(ENABLE);
}

uint8_t tick_check(void)
{
	if (tim4_overflow) {				// busy wait for flag
		tim4_overflow = 0;			// clear flag
		return 1;
	}
	return 0;
}
