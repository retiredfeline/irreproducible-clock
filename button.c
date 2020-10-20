#define STM8S103

#include <stm8s_gpio.h>

#include "button.h"

void button_init(void)
{
	GPIO_Init(GPIOA, GPIO_PIN_1, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOA, GPIO_PIN_2, GPIO_MODE_IN_PU_NO_IT);
}

uint8_t button_state(void)
{
	// BUTTON_HOURS is A1, BUTTON_MINS is A2
	return ~GPIO_ReadInputData(GPIOA) & B_BOTH;
}
