#include "fmath.h"

#ifdef __SDCC

/* ZXN: not yet wired up — these stubs keep the component linkable
 * under SDCC so other components can reference math.h without
 * pulling libm in. Replace with real implementations once the ZXN
 * compile recipe for -lm is added to compile.sh. */
float fsin(float x)       { (void)x; return 0.0f; }
float fcos(float x)       { (void)x; return 1.0f; }
float fsqrt(float x)      { (void)x; return 0.0f; }
float fabs_float(float x) { return x < 0.0f ? -x : x; }
float fpi(void)           { return 3.14159265f; }

#else

#include <math.h>

float fsin(float x)       { return sinf(x); }
float fcos(float x)       { return cosf(x); }
float fsqrt(float x)      { return sqrtf(x); }
float fabs_float(float x) { return x < 0.0f ? -x : x; }
float fpi(void)           { return 3.14159265f; }

#endif
