#include <stm8s_tim4.h>
#include <stm8s_itc.h>

extern void tim4_isr(void) __interrupt(ITC_IRQ_TIM4_OVF);
extern void tick_init(void);
extern uint8_t tick_check(void);
