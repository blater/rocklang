---
title: RTL Component — time (sleep)
category: generator
tags: [rtl, sleep, timing, delay, zxn, host]
sources: []
updated: 2026-04-25
status: current
---

# RTL Component: time (sleep)

`sleep(ms)` pauses execution for approximately `ms` milliseconds.

## Rock API

```rock
sleep(to_word(100));   // pause ~100 ms
```

| Parameter | Type | Meaning |
|-----------|------|---------|
| `ms` | `word` | Duration in milliseconds (0–65535) |

## Implementation

**Source:** `src/lib/time.{h,c}`

The Rock-facing name `sleep` maps to the C symbol `rock_sleep` to avoid a collision with the POSIX `sleep(unsigned int)` declaration in `unistd.h`. The generator emits calls to `rock_sleep` directly.

| Target | Mechanism |
|--------|-----------|
| ZXN | `z80_delay_ms(ms)` from z88dk `<z80.h>` — calibrated for 3.5 MHz. |
| Host | `usleep(ms * 1000)` from `<unistd.h>`. |

## ZXN accuracy note

`z80_delay_ms` is calibrated for 3.5 MHz (ZX 48K base speed). On Next Turbo (7/14/28 MHz), the delay runs proportionally shorter. A future vblank-sync primitive will provide a cycle-accurate delay tied to the 50 Hz hardware interrupt.

## Test Coverage

`test/tier1_test.rkr` — smoke test: `sleep(to_word(1))` returns normally.

See [[rtl-sound]] for `beep` (audio timing) and [[targets/zxn/tools/z88dk-z80-library]] for `z80_delay_ms` details.
