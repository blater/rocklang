---
title: "RTL Component: Keyboard Scanner"
category: concepts
tags: [rtl, keyboard, zxn, z80, builtins]
sources: [src/lib/keyboard.h, src/lib/keyboard.c, src/lib/zxn/keyboard.asm, test/keyboard_test.rkr]
updated: 2026-04-12
status: current
---

# RTL Component: Keyboard Scanner

First RTL component delivered under the [[rtl/rtl-overview]] conventions. Scans the ZX Spectrum 8×5 key matrix via port `$FE` with eight different high-byte row addresses and maintains a 40-byte per-key press-strength buffer.

## When to use this vs [[rtl/rtl-input]]

Rock ships **two independent keyboard components** that serve different needs. Pick the one that matches what you are building:

|  | `scan_keyboard` + `key_pressed` (this page) | `inkey` / `keypress` ([[rtl/rtl-input]]) |
|---|---|---|
| **Designed for** | Action games needing multiple simultaneous key-holds | Menus, text entry, "press any key" prompts |
| **Returns** | Raw per-key press state (40-byte buffer) | Decoded ASCII code |
| **Simultaneous keys** | ✅ yes — all 40 keys visible at once | ❌ no — only the most recent key |
| **Shift / Symbol-Shift** | Reported as their own keys | Consumed as modifiers by the ROM |
| **Auto-repeat** | ❌ no — you see raw held state | ✅ yes — ROM handles it |
| **Depends on ROM** | No — reads port `$FE` directly | Yes — needs the ROM interrupt handler |
| **Depends on interrupts** | No — caller drives `scan_keyboard()` | Yes — IM1 + ROM vector must be active |
| **Typical use** | `if key_pressed(KEY_Z) && key_pressed(KEY_X)` | `if keypress() == 13 then...` |

**Rule of thumb:** if the player could plausibly need to hold two keys at once (left+fire, jump+shoot, strafe+turn), use *this* component. If you just want the next character they typed, use [[rtl/rtl-input]]. Both can be used in the same program.

## Rock-facing API

```rock
scan_keyboard();                        // latch the matrix into internal buffer
byte k := key_pressed(to_byte(KEY_5));  // 0 == not pressed, >0 == press strength
```

Two builtins, both registered in `src/generator.c` alongside `peek`/`poke`:

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `scan_keyboard` | `void` | none | Read all 8 half-rows into the internal buffer |
| `key_pressed` | `byte` | `byte key_id` | Return press strength for `key_id` (0 on out-of-range) |

Key identifiers are `#define`d in `src/lib/keyboard.h`. The numbering matches the physical scan order used by the asm file, so a bulk copy of the buffer matches the constants 1-to-1.

| Constant | Value | | Constant | Value | | Constant | Value |
|----------|------:|---|----------|------:|---|----------|------:|
| `KEY_5`  | 0 | | `KEY_T`  | 10 | | `KEY_H`  | 25 |
| `KEY_4`  | 1 | | `KEY_R`  | 11 | | `KEY_J`  | 26 |
| `KEY_3`  | 2 | | `KEY_E`  | 12 | | `KEY_K`  | 27 |
| `KEY_2`  | 3 | | `KEY_W`  | 13 | | `KEY_L`  | 28 |
| `KEY_1`  | 4 | | `KEY_Q`  | 14 | | `KEY_ENT`| 29 |
| `KEY_6`  | 5 | | `KEY_Y`  | 15 | | `KEY_V`  | 30 |
| `KEY_7`  | 6 | | `KEY_U`  | 16 | | `KEY_C`  | 31 |
| `KEY_8`  | 7 | | `KEY_I`  | 17 | | `KEY_X`  | 32 |
| `KEY_9`  | 8 | | `KEY_O`  | 18 | | `KEY_Z`  | 33 |
| `KEY_0`  | 9 | | `KEY_P`  | 19 | | `KEY_SHF`| 34 |
|         |   | | `KEY_G`  | 20 | | `KEY_B`  | 35 |
|         |   | | `KEY_F`  | 21 | | `KEY_N`  | 36 |
|         |   | | `KEY_D`  | 22 | | `KEY_M`  | 37 |
|         |   | | `KEY_S`  | 23 | | `KEY_SYM`| 38 |
|         |   | | `KEY_A`  | 24 | | `KEY_SPC`| 39 |

`KEY_COUNT` (40) is the buffer length and the upper bound `key_pressed` checks against.

## Half-row scan layout

```
port byte  | bit 4 | bit 3 | bit 2 | bit 1 | bit 0 |
-----------+-------+-------+-------+-------+-------+
$F7        |   5   |   4   |   3   |   2   |   1   |
$EF        |   6   |   7   |   8   |   9   |   0   |
$FB        |   T   |   R   |   E   |   W   |   Q   |
$DF        |   Y   |   U   |   I   |   O   |   P   |
$FD        |   G   |   F   |   D   |   S   |   A   |
$BF        |   H   |   J   |   K   |   L   |  ENT  |
$FE        |   V   |   C   |   X   |   Z   |  SHF  |
$7F        |   B   |   N   |   M   |  SYM  |  SPC  |
```

Each call to `scan_keyboard()` reads all eight ports in sequence, inverts the bits so `1` means pressed, then for each pressed key increments that key's buffer byte (saturating at 255) and resets released keys to 0. The increment gives callers a rough debounce / hold-time value for free.

## Implementation layout

| File | Purpose |
|------|---------|
| `src/lib/keyboard.h` | Public declarations + `KEY_*` constants. Both targets use this. |
| `src/lib/keyboard.c` | `#ifdef __SDCC` split: ZXN declares `extern _ZK_BUFFER` and forwards `scan_keyboard`; host keeps a 40-byte zeroed static buffer and a no-op `scan_keyboard`. |
| `src/lib/zxn/keyboard.asm` | Z80 scanner, 40-byte `keybuffer`, `PUBLIC _scan_keyboard` and `PUBLIC _ZK_BUFFER`. |
| `test/keyboard_test.rkr` | Host-stub test: calls `scan_keyboard()`, asserts every key reads 0, asserts out-of-range `key_pressed(200) == 0`. |

## Host behaviour

The host target has no real hardware. `scan_keyboard()` is a no-op and the internal 40-byte buffer is always zero, so every `key_pressed` call returns 0. This is intentional — the host target exists so the compiler + builtin + link path can be exercised under gcc and the 200+ test suite stays green. Future components may choose to stub differently (e.g. simulate a frame counter), but the rule is that host builds must never crash or hang.

## SDCC symbol rule (again)

The buffer is declared in C as `extern unsigned char ZK_BUFFER[KEY_COUNT];` and referenced as `_ZK_BUFFER` in `keyboard.asm`. This matches SDCC's automatic underscore prefix; without it the link fails with `undefined symbol: _ZK_BUFFER`. See [[rtl/rtl-overview]] "Gotcha: SDCC symbol prefix".

## Verification

- `./run_tests.sh` — `keyboard_test` contributes 4 assertions, total 221 tests pass.
- `rock --target=zxn test/keyboard_test.rkr` — produces `keyboard_test.nex` (≈33 KB) with no errors, only pre-existing SDCC "unused `this`" warnings from `Assert.rkr`.
- Manual hardware check (optional): load the `.nex` in CSpect or on a real Next, verify `key_pressed(KEY_5)` returns non-zero while "5" is held.

## See Also

- [[rtl/rtl-input]] — the ASCII counterpart (`inkey`/`keypress`), for text and menus
- [[rtl/rtl-overview]] — conventions and the gotcha index
- [[targets/zxn/zxn-keyboard]] — hardware-level description of the matrix
- [[pasta80/pasta80-lessons-for-rock]] — lessons this design is built on
