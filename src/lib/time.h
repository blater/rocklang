#ifndef ROCK_TIME_H
#define ROCK_TIME_H

#include "typedefs.h"

/* Pause execution for `ms` milliseconds.
 *
 * ZXN: uses z88dk's `z80_delay_ms`, which is calibrated for 3.5 MHz.
 * On Next Turbo (14/28 MHz) the pause will run proportionally shorter.
 * A future vblank-sync primitive will replace this with a cycle-accurate
 * path using the 50 Hz interrupt.
 *
 * Host: nanosleep.
 */
/* Rock-facing name is `sleep`; the C symbol is renamed to avoid a
 * collision with POSIX unistd.h `sleep(unsigned int)`. The generator
 * emits calls to rock_sleep directly. */
void rock_sleep(word ms);

#endif /* ROCK_TIME_H */
