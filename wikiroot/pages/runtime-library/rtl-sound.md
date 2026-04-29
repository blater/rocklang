---
title: RTL Component — sound (beeper)
category: generator
tags: [rtl, sound, beep, audio, zxn, host]
sources: []
updated: 2026-04-25
status: current
---

# RTL Component: sound (beep)

`beep(freq, dur)` emits a square-wave tone on the ZX internal speaker (or a terminal bell on host).

## Rock API

```rock
beep(to_word(440), to_word(250));   // 440 Hz for 250 ms
```

| Parameter | Type | Meaning |
|-----------|------|---------|
| `freq` | `word` | Frequency in Hz (roughly 30–10 000) |
| `dur` | `word` | Duration in milliseconds |

## Implementation

**Source:** `src/lib/sound.{h,c}`

### ZXN path

Calls ROM BEEPER entry at `$03B5` directly. Arguments are converted from Hz/ms to the ROM's native DE/HL parameters:

```
DE = tone period  = 437500 / freq − 30
HL = period count = freq × duration(s)
```

Calibrated for 3.5 MHz. On Next Turbo (7/14/28 MHz), pitch and duration scale proportionally. Uses the file-scope scratch-byte pattern (stash args in static bytes, load with `LD A,(nn)`) to avoid SDCC calling-convention issues.

### Host path

Writes the terminal bell character (`'\a'`) to stdout — a best-effort stand-in. Frequency and duration are ignored.

## Notes

- `freq == 0` is a silent no-op on ZXN (division guard).
- `dur == 0` produces the shortest possible tone (`HL = 1`).
- Call `sleep()` separately if you need silence between tones.

## Test Coverage

`test/tier1_test.rkr` — smoke test: call returns normally under piped stdout.

See [[rtl-time]] for `sleep` (timing) and [[targets/zxn/zxn-sound]] for AY-3-8912 music.
