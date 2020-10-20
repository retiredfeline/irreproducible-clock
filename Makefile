#Compiler
CC=sdcc
CFLAGS=-mstm8
INCLUDES=-Iinclude -Ipt-1.4
LIBS=spl.lib

iclock.ihx:	clock.rel mcu.rel tick.rel tod.rel display.rel button.rel
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.rel:		%.c %.h
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $(<:.c=.rel)

%.flash:	%.ihx
	stm8flash -cstlinkv2 -pstm8s103f3 -w$(<:.flash=.ihx)

flash:	iclock.flash

clean:
	rm -f *.asm *.cdb *.lk *.lst *.map *.mem *.rel *.rst *.sym
