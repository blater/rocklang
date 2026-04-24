#include "fmath.h"

#ifdef __SDCC

#include <math.h>

float fsin(float x)       { return sin(x); }
float fcos(float x)       { return cos(x); }
float fsqrt(float x)      { return sqrt(x); }
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
