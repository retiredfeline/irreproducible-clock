#define STM8S103

#include <stm8s_clk.h>

#include "mcu.h"

void mcu_init(void)
{
	CLK_DeInit();
	CLK_HSICmd(ENABLE);				// internal oscillator
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);	// 16MHz
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);     
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, ENABLE);     
}

void mcu_enable_interrupts(void)
{
	enableInterrupts();				// global interrupts
}
