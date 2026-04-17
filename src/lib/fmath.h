#ifndef ROCK_FMATH_H
#define ROCK_FMATH_H

#include "typedefs.h"

/* Floating-point math primitives. Wraps libm on host. ZXN support is
 * deferred — SDCC's float library works but requires a different link
 * recipe and the ZXN compile path is not yet wired up in compile.sh.
 *
 * fsin/fcos/fsqrt — angles are in radians.
 * fabs_float     — absolute value.
 * fpi            — constant π returned as a Rock float.
 *
 * Names are f-prefixed to avoid colliding with libm's sin/cos/sqrt
 * symbols and to match the pasta80 Real math convention.
 */

float fsin(float x);
float fcos(float x);
float fsqrt(float x);
float fabs_float(float x);
float fpi(void);

#endif /* ROCK_FMATH_H */
