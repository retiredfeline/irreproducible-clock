enum button_bits { B_NONE = 0, B_0 = 0x2, B_1 = 0x4, B_BOTH = (B_0 | B_1) };

extern void button_init(void);
extern uint8_t button_state(void);
