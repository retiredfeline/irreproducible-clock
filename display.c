#define STM8S103

#include <stm8s_gpio.h>

#include "clock.h"

#include "display.h"

#define	DIGITS		3
#define HSEGMENTS	0x54	// G.E.C..
#define	MSEGMENTS	0x23	// .F...BA
#define	DP_COLON_POS	0

static uint8_t hours_buffer[3];
static uint8_t mins_buffer[3];
static uint8_t *display_ptr;
static uint8_t digit_number;
const static uint8_t font[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
static uint8_t visible;

// ports and pins are discontinous due to limited pins of TSSOP20 package

// segments are listed from LSb = A, to MSb = DP
const static port_pin segments[] = {
	{ GPIOD, GPIO_PIN_5 },
	{ GPIOD, GPIO_PIN_6 },
	{ GPIOC, GPIO_PIN_3 },
	{ GPIOC, GPIO_PIN_4 },
	{ GPIOC, GPIO_PIN_5 },
	{ GPIOC, GPIO_PIN_6 },
	{ GPIOC, GPIO_PIN_7 },
	{ GPIOA, GPIO_PIN_3 },
};

#define	NSEGMENTS	((sizeof segments)/(sizeof segments[0]))

// digits are listed from left to right as strings are too
// D1 is not used unless necessary as it's the SWIM pin
const static port_pin digits[] = {
	{ GPIOD, GPIO_PIN_4 },
	{ GPIOD, GPIO_PIN_3 },
	{ GPIOD, GPIO_PIN_2 },
};

#define	NDIGITS		((sizeof digits)/(sizeof digits[0]))

#define	GPIOWriteValue(port,pin,val)	((val) ? GPIO_WriteHigh(port,pin) : GPIO_WriteLow(port,pin))

void display_set_ptr(enum display_mode mode)
{
	if (mode == D_HOURS)
		display_ptr = hours_buffer;
	else if (mode == D_MINS)
		display_ptr = mins_buffer;
}

void display_init(void)
{
	GPIO_DeInit(GPIOB);
	GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_HIGH_SLOW);        // LED
	// segment pins
	for (uint8_t i = 0; i < NSEGMENTS; i++) {
		GPIO_Init(segments[i].port, segments[i].pin, GPIO_MODE_OUT_PP_LOW_SLOW);
	}
	// digit pins
	for (uint8_t i = 0; i < NDIGITS; i++) {
		GPIO_Init(digits[i].port, digits[i].pin, GPIO_MODE_OUT_PP_LOW_SLOW);
	}
	digit_number = NDIGITS - 1;
	hours_buffer[2] = HSEGMENTS;
	mins_buffer[2] = MSEGMENTS;
	display_set_ptr(D_HOURS);
}

void display_flip_b5(void)
{
	GPIO_WriteReverse(GPIOB, GPIO_PIN_5);
}

static void digits_update(uint8_t *buffer, uint8_t value)
{
	uint8_t	tens = value / 10;
	uint8_t units = value % 10;

	buffer[0] = font[tens];
	buffer[1] = font[units];
}

void display_update(void)
{
	digits_update(hours_buffer, TIME.hours);
	digits_update(mins_buffer, TIME.minutes);
}

void display_update_dot(void)
{
	display_ptr[DP_COLON_POS] ^= 0x80;	// flip dot
}

static void display_segments(uint8_t value)
{
	const port_pin	*pp = &segments[NSEGMENTS-1];

	// set pins from MSb down to LSb
	for (uint8_t mask = 0x80; mask != 0; mask >>= 1, pp--) {
		GPIOWriteValue(pp->port, pp->pin, value & mask);
	}
}

void display_set_visibility(uint8_t state)
{
	visible = state;
}

void display_next_digit(void)
{
	// turn off digit line
	GPIO_WriteLow(digits[digit_number].port, digits[digit_number].pin);
	// advance digit
	digit_number++;
	if (digit_number >= NDIGITS)
		digit_number = 0;
	// set segments
	display_segments(display_ptr[digit_number]);
	// turn on digit line
	if (visible)
		GPIO_WriteHigh(digits[digit_number].port, digits[digit_number].pin);
}

uint8_t display_digit_number(void)
{
	return digit_number;
}
