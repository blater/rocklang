#include "cls.h"

#ifdef __SDCC

/* ZX ROM CLS entry point. Clears the upper screen and fills the
 * attribute area with the current PAPER/INK (from ATTR_P sysvar). */
void cls(void) {
  __asm
    call 0x0DAF
  __endasm;
}

#else

#include <stdio.h>
#include "host_caps.h"
#include "termbox2.h"

void cls(void) {
  if (host_caps.print_at) {
    tb_clear();
    tb_present();
    return;
  }
  /* Fallback: form-feed so piped stdout stays deterministic-ish. */
  putchar('\f');
  fflush(stdout);
}

#endif
