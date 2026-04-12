#ifndef _LIB_KEYBOARD_H
#define _LIB_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// Update keyboard state from hardware
// Call this regularly (e.g., from interrupt handler or main loop)
void scan_keyboard(void);

// each var corresponds to a memory mapped keypress status
// populated by keyboard.asm _scan_keyboard
extern uint8_t KEY0;
extern uint8_t KEY9;
extern uint8_t KEY8;
extern uint8_t KEY7;
extern uint8_t KEY6;

extern uint8_t KEY5;
extern uint8_t KEY4;
extern uint8_t KEY3;
extern uint8_t KEY2;
extern uint8_t KEY1;

extern uint8_t KEYT;
extern uint8_t KEYR;
extern uint8_t KEYE;
extern uint8_t KEYW;
extern uint8_t KEYQ;

extern uint8_t KEYY;
extern uint8_t KEYU;
extern uint8_t KEYI;
extern uint8_t KEYO;
extern uint8_t KEYP;

extern uint8_t KEYG;
extern uint8_t KEYF;
extern uint8_t KEYD;
extern uint8_t KEYS;
extern uint8_t KEYA;

extern uint8_t KEYH;
extern uint8_t KEYJ;
extern uint8_t KEYK;
extern uint8_t KEYL;
extern uint8_t KEYENT;

extern uint8_t KEYV;
extern uint8_t KEYC;
extern uint8_t KEYX;
extern uint8_t KEYZ;
extern uint8_t KEYSHF;

extern uint8_t KEYB;
extern uint8_t KEYN;
extern uint8_t KEYM;
extern uint8_t KEYSYM;
extern uint8_t KEYSPC;

#endif
