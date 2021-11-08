#define STM8S103

#include <stm8s_i2c.h>

#include "rtc.h"

static uint8_t now[7];
#define	SECONDS_POS	0
#define	MINUTES_POS	1
#define	HOURS_POS	2
struct rtc_time rtc_time;

#define	DS3231ADDR	0x68u

void rtc_init(void)
{
	rtc_time.hours = 12;
	rtc_time.minutes = 34;
	rtc_time.seconds = 56;
	I2C_DeInit();
	// Set up for 100kHz, own address irrelevant, DutyCycle irrelevant,
	// Ack curr, 7 bit addresses, 16 MHz
	I2C_Init(100000UL, DS3231ADDR << 1, I2C_DUTYCYCLE_2, I2C_ACK_CURR,
		I2C_ADDMODE_7BIT, 16);
	I2C_Cmd(ENABLE);
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
	// send pointer of 0
	I2C_GenerateSTART(ENABLE);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(DS3231ADDR << 1, I2C_DIRECTION_TX);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
	I2C_SendData(0);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;

	I2C_GenerateSTART(ENABLE);			// generates a restart
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(DS3231ADDR << 1, I2C_DIRECTION_RX);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		;
	// read sizeof(now) (7) bytes
	I2C_AcknowledgeConfig(I2C_ACK_CURR);		// ACK
	uint8_t i = 0;
	while (i < sizeof(now) - 3) {
		while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED))
			;
		now[i] = I2C_ReceiveData();
		i++;
	}
	// third last byte
	// wait for BTF
	while (I2C_GetFlagStatus(I2C_FLAG_TRANSFERFINISHED) == RESET)
		;
	I2C_AcknowledgeConfig(I2C_ACK_NONE);		// NAK
	now[i] = I2C_ReceiveData();			// N-2
	i++;
	// second last byte
	I2C_GenerateSTOP(ENABLE);
	now[i] = I2C_ReceiveData();			// N-1
	i++;
	// last byte
	// wait for RXNE
	while (I2C_GetFlagStatus(I2C_FLAG_RXNOTEMPTY) == RESET)
		;
	now[i] = I2C_ReceiveData();			// N
	unpack_time();
}

static void update_element(uint8_t value, uint8_t reg)
{
	// send data of register then value
	I2C_GenerateSTART(ENABLE);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(DS3231ADDR << 1, I2C_DIRECTION_TX);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
	I2C_SendData(reg);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;
	I2C_SendData(value);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;
	I2C_GenerateSTOP(ENABLE);
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
