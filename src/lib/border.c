#include "border.h"

/* Shadow of the last colour passed to border(). Read back by border_get()
 * on both targets — ZXN cannot read the border back from hardware, so the
 * shadow is authoritative on both platforms. */
static byte last_border = 0;

#ifdef __SDCC

#include <z80.h>

void border(byte colour) {
  last_border = colour & 0x07;
  z80_outp(0xFE, last_border);
}

#else

void border(byte colour) {
  last_border = colour & 0x07;
  /* Host: no hardware. Shadow-only. */
}

#endif

byte border_get(void) {
  return last_border;
}
