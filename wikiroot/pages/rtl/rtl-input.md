---
title: "RTL Component: ASCII Input (inkey / keypress)"
category: concepts
tags: [rtl, keyboard, input, zxn, rom, builtins]
sources: [src/lib/input.h, src/lib/input.c, test/tier1_test.rkr]
updated: 2026-04-13
status: current
---

# RTL Component: ASCII Input (inkey / keypress)

Second keyboard-related component, serving a **completely different need** from [[rtl/rtl-keyboard]]. This one hands back a proper ASCII byte decoded by the ROM (Shift, Symbol Shift, caps lock, auto-repeat, the lot); the other exposes raw matrix state for detecting multiple simultaneous key-holds.

**Use this component for:** menus, text entry, "press a key to start", typing games, anything where you want a character.

**Use [[rtl/rtl-keyboard]] instead for:** action games where the player holds Z+X+SPACE at the same time, joystick-style controls, per-frame polling.

Both components ship, both are registered as builtins, and they can be used together in the same program.

## Rock-facing API

```rock
byte k := inkey();        // non-blocking: 0 if no key, else ASCII
byte c := keypress();     // blocking: waits until a key, returns ASCII
```

| Name | Return | Args | Blocking | Purpose |
|------|--------|------|----------|---------|
| `inkey`    | `byte` | none | No  | Return ASCII of current key, 0 if none |
| `keypress` | `byte` | none | Yes | Wait for a key, return its ASCII |

## How it works (ZXN)

Both routines lean on the standard ZX ROM key-scan machinery, which is running out-of-the-box on every z88dk `+zxn -startup=1` build:

1. **IM1 is enabled** by the CRT at program start. The Z80 takes an interrupt 50 times/second.
2. **The ROM interrupt handler at `$0038`** (vectored through IM1) scans the keyboard matrix, decodes Shift / Symbol-Shift / caps lock, handles auto-repeat, and writes the resulting ASCII code into sysvar **`LAST_K` ($5C08)**. It also sets **bit 5 of `FLAGS` (`IY+1`)** as a "new key ready" flag.
3. **`inkey`** tests `FLAGS` bit 5. If set, it clears the bit and returns `LAST_K`. Otherwise returns 0.
4. **`keypress`** clears `FLAGS` bit 5 to discard any stale press, then `HALT`s until the next interrupt, tests the flag again, and loops until a fresh key arrives.

This is exactly the idiom ZX BASIC's `INKEY$` uses and exactly what [[pasta80/pasta80-rtl-architecture]] does in `zxrom.asm`. There is no Z80 code we own here — the ROM does the hard work.

## Dependency: ROM must be paged in and IM1 must be active

Because we lean on the ROM interrupt handler, this component breaks silently if a Rock program:

- Pages the Spectrum ROM out of `$0000-$3FFF` (to use that RAM), or
- Switches to IM2 with a custom vector table (no ROM handler running), or
- Disables interrupts for extended periods.

None of these are possible from today's Rock surface — there is no `page_rom()` or `set_im2()` builtin. But the moment those land (they will, once we care about sound loops and raster effects) this component needs a companion: either a Rock-owned IM2 handler that preserves ROM key-scan semantics, or a fallback scanner that decodes the matrix ourselves.

Until that day, the dependency is harmless and documented.

## Why we don't just use this for everything

The ROM route is lovely for anything character-oriented, but it has hard limits that make it unsuitable for action games:

- **Only one key at a time.** `LAST_K` holds the most recent ASCII code the ROM decoded. If the player holds left+fire, you see one of them.
- **No press/release edges.** There is no way to ask "is A currently held down?"
- **Auto-repeat gets in the way.** Held keys re-trigger every ~30 frames, not every frame.
- **Shift and Symbol-Shift are consumed as modifiers.** You can't tell the ROM "treat SHIFT as its own key" — it's baked into decoding.

For real action games you want to see `left`, `right`, `fire`, and `jump` all held simultaneously, with no auto-repeat and no modifier semantics. That's what [[rtl/rtl-keyboard]]'s `scan_keyboard()` + `key_pressed(KEY_X)` gives you.

## Picking a mechanism

| You want... | Use | Why |
|---|---|---|
| "Press any key to start" | `keypress()` | blocking + ASCII |
| Single-line text entry | `inkey()` in a loop, plus a future `input()` | character-oriented |
| Menu navigation (up/down/enter) | `inkey()` | discrete keystrokes |
| WASD movement | either, but `key_pressed` scales better to multi-key | needs held-key state |
| Fire + move at the same time | [[rtl/rtl-keyboard]] `scan_keyboard` + `key_pressed` | simultaneous keys |
| Action games with jump+shoot+run | [[rtl/rtl-keyboard]] | ROM route can't do it |

## Host behaviour

The host target uses termbox2 (when the capability is active): `tb_peek_event` for non-blocking `inkey`, `tb_poll_event` for blocking `keypress`. A small mapping table converts common non-printable events (Enter, Space, Tab, Backspace, Escape) to their ASCII equivalents so Rock programs see the same bytes on both targets.

When the capability is off (piped stdout under `run_tests.sh`), `inkey` returns 0 and `keypress` falls back to `getchar()`.

## Implementation layout

| File | Purpose |
|------|---------|
| `src/lib/input.h` | Public declarations |
| `src/lib/input.c` | `#ifdef __SDCC` split: ZXN uses inline-asm `HALT`/`bit 5,(iy+1)`/`LAST_K`; host uses termbox2 events with `getchar` fallback |
| `test/tier1_test.rkr` | Host smoke test for the non-blocking path |

No separate `.asm` file — inline asm inside the C shim is enough because the ROM is doing the work.

## See Also

- [[rtl/rtl-keyboard]] — the matrix scanner, for simultaneous key-hold detection
- [[rtl/rtl-overview]] — conventions
- [[rtl/rtl-print-at]] — also calls ROM via `RST 10h`; same "depends on ROM paged in" caveat
- [[pasta80/pasta80-target-platforms]] — pasta80's `zx_readkey` / `zx_testkey`, the idiom we adopted
