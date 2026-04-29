---
title: RTL Component — helpers (scalar utilities)
category: generator
tags: [rtl, helpers, bit-ops, char, abs, odd, even, hi, lo, swap]
sources: []
updated: 2026-04-25
status: current
---

# RTL Component: helpers

A bundle of small scalar utilities with no platform dependencies. All functions are pure — no state, no I/O, no target-specific code.

## Rock API

### Ordinal predicates

```rock
byte o := odd(n);    // 1 if n is odd, 0 otherwise
byte e := even(n);   // 1 if n is even, 0 otherwise
```

### Byte/word splitting

```rock
byte h := hi(to_word(4660));    // high byte → 18  (0x12)
byte l := lo(to_word(4660));    // low byte  → 52  (0x34)
word s := swap(to_word(4660));  // byte-swap → 13330 (0x3412)
```

### Character case conversion (ASCII only)

```rock
char u := upcase('a');   // → 'A'
char d := locase('Z');   // → 'z'
// Non-letter characters are returned unchanged
```

### Absolute value

```rock
int  p := abs_int(0 - 7);        // → 7
word q := abs_word(to_word(42)); // → 42 (identity: words are unsigned)
```

## Full function signatures

| Function | Parameters | Return | Notes |
|----------|-----------|--------|-------|
| `odd(x)` | `int` | `byte` | 1 if x & 1 |
| `even(x)` | `int` | `byte` | 1 if (x & 1) == 0 |
| `hi(w)` | `word` | `byte` | `(w >> 8) & 0xFF` |
| `lo(w)` | `word` | `byte` | `w & 0xFF` |
| `swap(w)` | `word` | `word` | `(lo << 8) \| hi` |
| `upcase(c)` | `char` | `char` | 'a'–'z' → 'A'–'Z' |
| `locase(c)` | `char` | `char` | 'A'–'Z' → 'a'–'z' |
| `abs_int(x)` | `int` | `int` | `x < 0 ? -x : x` |
| `abs_word(w)` | `word` | `word` | identity (unsigned) |

## Implementation

**Source:** `src/lib/helpers.{h,c}`

Single-file implementation; no `#ifdef __SDCC` split needed. These are trivial inline-friendly one-liners — identical on all targets. Parity with the PASTA/80 `Hi`/`Lo`/`Swap`/`Odd`/`Even` utility bundle.

## Test Coverage

`test/helpers_test.rkr` — all nine functions, including numeric edge cases (0x1234 split, swap symmetry, abs of 0 and negatives, case identity for non-letters).
