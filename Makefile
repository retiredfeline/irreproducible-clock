# Compiler
CC=sdcc
CFLAGS=-mstm8
INCLUDES=-Iinclude -Ipt-1.4
LIBS=spl.lib
DDS_ADJ=0ul

# Add DS3231=[is] to make invocation to build with rtc module instead of tod module
ifdef DS3231
CFLAGS+=-DDS3231
# if set to s, use software i2c gpio implementation
ifeq "$(DS3231)" "s"
TIMEBASE_OBJ=rtcsoft.rel
else
TIMEBASE_OBJ=rtc.rel
endif
else
TIMEBASE_OBJ=tod.rel
endif

iclock.ihx:	clock.rel mcu.rel tick.rel display.rel button.rel $(TIMEBASE_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Remake tod.rel if DDS_ADJ changed in Makefile
tod.rel:	tod.c tod.h Makefile
	$(CC) -c $(CFLAGS) -DDDS_ADJ=$(DDS_ADJ) $(INCLUDES) $< -o $(<:.c=.rel)

rtcsoft.rel:	rtcsoft.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $(<:.c=.rel)

%.rel:		%.c %.h
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $(<:.c=.rel)

%.flash:	%.ihx
	stm8flash -cstlinkv2 -pstm8s103f3 -w$(<:.flash=.ihx)

flash:	iclock.flash

clean:
	rm -f *.asm *.cdb *.lk *.lst *.map *.mem *.rel *.rst *.sym
