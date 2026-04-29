#include "sound.h"

#ifdef __SDCC

/* Call ROM BEEPER at $03B5 directly. Parameters per the 48K ROM:
 *   DE = tone period = 437500 / freq - 30
 *   HL = number of tone periods to emit = freq * duration(sec)
 *
 * Calibrated for 3.5 MHz. Turbo modes scale both pitch and duration. */

static unsigned int beep_de;
static unsigned int beep_hl;

static void beeper_call(void) {
  __asm
    ld de, (_beep_de)
    ld hl, (_beep_hl)
    call 0x03B5
  __endasm;
}

void beep(word freq, word dur) {
  unsigned int pitch;
  unsigned int len;
  if (freq == 0) return;
  pitch = (unsigned int)(437500UL / freq);
  if (pitch > 30) pitch -= 30; else pitch = 1;
  len = (unsigned int)(((unsigned long)freq * dur) / 1000UL);
  if (len == 0) len = 1;
  beep_de = pitch;
  beep_hl = len;
  beeper_call();
}

#else

#include <stdio.h>

void beep(word freq, word dur) {
  (void)freq;
  (void)dur;
  putchar('\a');
  fflush(stdout);
}

#endif
