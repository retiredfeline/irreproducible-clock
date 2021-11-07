#define STM8S103

#include <stm8s_tim1.h>
#include <stm8s_gpio.h>

#include "rtc.h"

static uint8_t now[7];
#define	SECONDS_POS	0
#define	MINUTES_POS	1
#define	HOURS_POS	2
struct rtc_time rtc_time;

#define	DS3231ADDR	0x68u

//
// I2C communication functions using bit-banging
// I have implemented only what I needed for my project
// In particular, clock stretching is not implemented
// Feel free to improve
//

#define	SDA		GPIOB,GPIO_PIN_5
#define	SCL		GPIOB,GPIO_PIN_4
#define	WriteLow(p)	GPIO_WriteLow(p)	
#define	WriteHigh(p)	GPIO_WriteHigh(p)	
#define ReadPin(p)	GPIO_ReadInputPin(p)

static uint8_t address2;

static void delayxus()
{
	// (clock CPU/16)/x, put x-1
	TIM1_TimeBaseInit(16, TIM1_COUNTERMODE_DOWN, 9, 0);
	TIM1_Cmd(ENABLE);
	while (!TIM1_GetFlagStatus(TIM1_FLAG_UPDATE))
		;
	TIM1_ClearFlag(TIM1_FLAG_UPDATE);
	TIM1_Cmd(DISABLE);
}

static void i2cinit(uint8_t a)
{
	address2 = a << 1;
	GPIO_Init(SCL, GPIO_MODE_OUT_OD_HIZ_SLOW);
	GPIO_Init(SDA, GPIO_MODE_OUT_OD_HIZ_SLOW);
	TIM1_DeInit();
	WriteHigh(SDA);
	delayxus();
	WriteHigh(SCL);
	delayxus();
}

static void i2cstart()
{
	WriteLow(SDA);
	delayxus();
	WriteLow(SCL);
	delayxus();
}

static void i2crestart()
{
	WriteHigh(SDA);
	delayxus();
	WriteHigh(SCL);
	delayxus();
	WriteLow(SDA);
	delayxus();
	WriteLow(SCL);
	delayxus();
}

static void i2cstop()
{
	WriteLow(SCL);
	delayxus();
	WriteLow(SDA);
	delayxus();
	WriteHigh(SCL);
	delayxus();
	WriteHigh(SDA);
	delayxus();
}

static void i2cack()
{
	WriteLow(SDA);
	delayxus();
	WriteHigh(SCL);
	delayxus();
	WriteLow(SCL);
	delayxus();
	WriteHigh(SDA);
	delayxus();
}

static void i2cnak()
{
	WriteHigh(SDA);
	delayxus();
	WriteHigh(SCL);
	delayxus();
	WriteLow(SCL);
	delayxus();
	WriteHigh(SDA);
	delayxus();
}

static uint8_t i2csend(unsigned char data)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		if (data & 0x80)
			WriteHigh(SDA);
		else
			WriteLow(SDA);
		delayxus();
		WriteHigh(SCL);
		delayxus();
		WriteLow(SCL);
		delayxus();
		data <<= 1;
	}
	WriteHigh(SDA);
	delayxus();
	WriteHigh(SCL);
	delayxus();
	GPIO_Init(SDA, GPIO_MODE_IN_FL_NO_IT);
	i = ReadPin(SDA) ? 1 : 0;
	delayxus();
	WriteLow(SCL);
	delayxus();
	GPIO_Init(SDA, GPIO_MODE_OUT_OD_HIZ_SLOW);
	return i;
}

static uint8_t i2csendaddr()
{
	return i2csend(address2);
}

static uint8_t i2creadaddr()
{
	return i2csend(address2 | 1);
}

static uint8_t i2cread()
{
	uint8_t data = 0;

	GPIO_Init(SDA, GPIO_MODE_IN_FL_NO_IT);
	for (uint8_t i = 0; i < 8; i++) {
		data <<= 1;
		data |= ReadPin(SDA) ? 1 : 0;
		WriteHigh(SCL);
		delayxus();
		WriteLow(SCL);
		delayxus();
	}
	GPIO_Init(SDA, GPIO_MODE_OUT_OD_HIZ_SLOW);
	return data;
}

void rtc_init(void)
{
	i2cinit(DS3231ADDR);
}

static uint8_t bcd2bin(uint8_t c)
{
	return c - 6 * (c >> 4);
}

static void unpack_time(void)
{
	rtc_time.changed = T_NONE;
	uint8_t hours = bcd2bin(now[HOURS_POS] & 0x3f);
	if (hours >= 24)
		hours = 24;		// flag problem
	if (hours != rtc_time.hours) {
		rtc_time.hours = hours;
		rtc_time.changed |= T_HOURS;
	}
	uint8_t minutes = bcd2bin(now[MINUTES_POS]);
	if (minutes >= 60)
		minutes = 60;		// flag problem
	if (minutes != rtc_time.minutes) {
		rtc_time.minutes = minutes;
		rtc_time.changed |= T_MINUTES;
	}
	uint8_t seconds = bcd2bin(now[SECONDS_POS]);
	if (seconds >= 60)
		seconds = 60;		// flag problem
	if (seconds != rtc_time.seconds) {
		rtc_time.seconds = seconds;
		rtc_time.changed |= T_SECONDS;
	}
}

void rtc_getnow(void)
{
	i2cstart();
	i2csendaddr();
	i2csend(0);
	i2crestart();
	i2creadaddr();
	uint8_t *p = now;
	for (uint8_t i = 0; i < sizeof(now) - 1; i++) {
		*p++ = i2cread();
		i2cack();
	}
	*p++ = i2cread();
	i2cnak();
	i2cstop();
	unpack_time();
}

static void update_element(uint8_t value, uint8_t reg)
{
	i2cstart();
	i2csendaddr();
	i2csend(reg);
	i2csend(value);
	i2cstop();
}

static uint8_t bin2bcd(uint8_t c)
{
	return c + 6 * (c / 10);
}

void rtc_update(enum time_changes changed)
{
	switch(changed) {
	case T_HOURS:
		update_element(bin2bcd(rtc_time.hours), HOURS_POS);
		break;
	case T_MINUTES:
		update_element(bin2bcd(rtc_time.minutes), MINUTES_POS);
		update_element(0x00, SECONDS_POS);	// seconds <- 0
		break;
	}
}
