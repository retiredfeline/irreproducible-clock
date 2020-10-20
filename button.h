enum button_bits { B_NONE = 0, B_HOURS = 0x2, B_MINS = 0x4, B_BOTH = (B_HOURS | B_MINS) };

extern void button_init(void);
extern uint8_t button_state(void);
