---
title: RTL Component — random
category: generator
tags: [rtl, random, rng, lcg, zxn, host]
sources: []
updated: 2026-04-25
status: current
---

# RTL Component: random

Three builtins for pseudo-random number generation.

## Rock API

```rock
randomize();                        // reseed from platform entropy
byte b := random_byte(to_byte(6));  // 0..5
word w := random_word(to_word(256));// 0..255
```

| Call | Return | Meaning |
|------|--------|---------|
| `randomize()` | `void` | Reseed the internal RNG from platform entropy |
| `random_byte(max)` | `byte` | Random value in `0..max-1`; returns `0` if `max == 0` |
| `random_word(max)` | `word` | Random value in `0..max-1`; returns `0` if `max == 0` |

## Implementation

**Source:** `src/lib/random.{h,c}`

### Generator

16-bit linear congruential generator (LCG) with Numerical Recipes constants:

```
state = state × 25173 + 13849   (mod 2¹⁶)
```

State is a file-scope `word` (`rock_rng_state`), shared between host and ZXN. Without a prior `randomize()` call the seed is `1`, giving a deterministic sequence — handy for tests.

### randomize()

| Target | Mechanism |
|--------|-----------|
| ZXN | Reads the Z80 `R` (memory refresh) register — a low-entropy but cheap hardware source. Seeds `rock_rng_state` from the low 7 bits of R, then advances once to avoid state 0. |
| Host | `rand() & 0xFFFF` — seeds from the host C runtime. Deterministic without a prior `srand`; exists for API parity rather than real entropy. |

### Bounds

`random_byte(max)` and `random_word(max)` return `rng_next() % max`. Distribution is uniform for powers of two; for other `max` values there is a small modulo bias (acceptable for games and demos).

## Test Coverage

`test/random_test.rkr` — deterministic-seed run (no `randomize()` call) verifying 500 draws stay within `[0, max)` for both byte and word; smoke-tests `randomize()` for crash safety.
