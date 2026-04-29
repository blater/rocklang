#ifndef ROCK_INPUT_H
#define ROCK_INPUT_H

#include "typedefs.h"

/* Single-key input.
 *
 * inkey()    — non-blocking. Returns the ASCII code of the currently
 *              pressed key, or 0 if no key is held.
 * keypress() — blocking. Waits until a key is pressed and returns its
 *              ASCII code.
 */
byte inkey(void);
byte keypress(void);

#endif /* ROCK_INPUT_H */
