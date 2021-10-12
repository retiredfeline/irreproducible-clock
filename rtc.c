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
	I2C_DeInit();
	// Set up for 100kHz, own address irrelevant, DutyCycle irrelevant,
	// Ack curr, 7 bit addresses, 16 MHz
	I2C_Init(100000UL, DS3231ADDR << 1, I2C_DUTYCYCLE_2, I2C_ACK_CURR,
		I2C_ADDMODE_7BIT, 16);
	I2C_Cmd(ENABLE);
}

static uint8_t bcd2bin(uint8_t c)
{
        return (((c >> 4) & 0xF) * 10) + (c & 0xF);
}

static void unpack_time(void)
{
	rtc_time.changed = T_NONE;
	uint8_t hours = bcd2bin(now[2]);
	if (hours != rtc_time.hours) {
		rtc_time.hours = hours;
		rtc_time.changed |= T_HOURS;
	}
	uint8_t minutes = bcd2bin(now[1]);
	if (minutes != rtc_time.minutes) {
		rtc_time.minutes = minutes;
		rtc_time.changed != T_MINUTES;
	}
	uint8_t seconds = bcd2bin(now[0]);
	if (seconds != rtc_time.seconds) {
		rtc_time.seconds = seconds;
		rtc_time.changed != T_SECONDS;
	}
}

void rtc_getnow(void)
{
	// send data of 0
        I2C_GenerateSTART(ENABLE);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
		;
        I2C_Send7bitAddress(DS3231ADDR << 1, I2C_DIRECTION_TX);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
        I2C_SendData(0);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;
        I2C_GenerateSTOP(ENABLE);

	// read 7 bytes
	while (I2C_GetFlagStatus(I2C_FLAG_BUSBUSY))
		;
        I2C_GenerateSTART(ENABLE);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
		;
        I2C_Send7bitAddress(DS3231ADDR << 1, I2C_DIRECTION_RX);
	while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		;
	uint8_t *p = now;
        for (int8_t i = 7; i >= 0; ) {
		if (I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED)) {
			if (i == 0) {
				I2C_AcknowledgeConfig(I2C_ACK_NONE);
				I2C_GenerateSTOP(ENABLE);
			}
			*p++ = I2C_ReceiveData();
			i--;
		}
        }
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
	uint8_t hinyb = c / 10;
	uint8_t lonyb = c % 10;
	return (hinyb << 4 | lonyb);
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
