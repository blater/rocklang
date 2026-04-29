---
title: RTL Component — fmath (float type and math)
category: generator
tags: [rtl, float, fmath, sin, cos, sqrt, math, zxn, host]
sources: []
updated: 2026-04-25
status: current
---

# RTL Component: fmath (float and math)

The `float` type and five floating-point math builtins.

## Rock API

### The float type

```rock
float x;               // default-initialised to 0.0
float y := 3.14;       // decimal literal
float z := y * 2.0;    // arithmetic: +, -, *, /
x := to_float(n);      // cast from int/byte/word
int i := to_int(x);    // truncating cast back to int
string s := toString(x);  // "3.14", "0", "-2.5"
float[] arr;           // float arrays supported
```

### Math builtins

```rock
float s := fsin(x);         // sine
float c := fcos(x);         // cosine
float r := fsqrt(16.0);     // square root → 4.0
float a := fabs_float(x);   // absolute value
float p := fpi();           // π ≈ 3.14159265
```

| Function | Signature | Notes |
|----------|-----------|-------|
| `fsin(x)` | `float → float` | sine |
| `fcos(x)` | `float → float` | cosine |
| `fsqrt(x)` | `float → float` | square root |
| `fabs_float(x)` | `float → float` | absolute value (f-prefixed to avoid libm collision) |
| `fpi()` | `void → float` | π constant |

## Implementation

**Source:** `src/lib/fmath.{h,c}`

| Target | Library |
|--------|---------|
| Host (gcc) | `<math.h>` — `sinf`, `cosf`, `sqrtf` |
| ZXN (SDCC) | `<math.h>` — z88dk genmath via `-lm`; uses `sin`, `cos`, `sqrt` (z88dk's genmath float is single-precision) |

`fabs_float` is implemented directly as `x < 0.0f ? -x : x` on both targets — no libm `fabsf` call — to avoid symbol conflicts. Names are f-prefixed throughout to prevent collisions with libm symbols that the SDCC math library may export at the same names.

## Type mapping

`float` in Rock maps to C `float` (single-precision IEEE 754 on host; z88dk genmath 32-bit float on ZXN — same bit width, same range).

## ZXN notes

z88dk's genmath float library is complete for ZXN (was previously deferred; now included in the build). Link with `-lm` via the `rock` build script's zcc invocation.

## Test Coverage

`test/float_test.rkr` — default initialisation, literals, arithmetic, `to_float`/`to_int` round-trips, all five math builtins, `toString(float)`, and `float[]` arrays.
