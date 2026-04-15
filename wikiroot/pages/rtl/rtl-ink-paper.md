---
title: "RTL Component: ink / paper / bright / flash (planned)"
category: concepts
tags: [rtl, colour, attributes, zxn, planned]
sources: [src/lib/print_at.c, wikiroot/pages/rtl/rtl-overview.md, wikiroot/pages/rtl/rtl-print-at.md]
updated: 2026-04-13
status: current
---

# RTL Component: ink / paper / bright / flash (planned)

Planned RTL component adding ZX Spectrum character-cell colour control. Owns the current attribute state; `print` (host branch) reads it back when drawing.

**Status:** design only. No code written yet.

## Rock-facing API

```rock
ink(to_byte(COLOUR_RED));
paper(to_byte(COLOUR_WHITE));
bright(to_byte(1));
print(to_byte(10), to_byte(5), "hello");   // red-on-white, bright
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `ink`     | `void` | `byte colour` | Set foreground colour for subsequent prints. |
| `paper`   | `void` | `byte colour` | Set background colour for subsequent prints. |
| `bright`  | `void` | `byte on`     | 0 = normal, 1 = bright. |
| `flash`   | `void` | `byte on`     | 0 = static, 1 = flashing. |
| `inverse` | `void` | `byte on`     | 0 = normal, 1 = swap ink/paper on subsequent output. |
| `over`    | `void` | `byte on`     | 0 = overwrite, 1 = XOR-combine with existing cell. |

**Colour constants** (standard Spectrum palette — `#define` in the public header per [[rtl/rtl-overview]] rule 5):

```
COLOUR_BLACK 0   COLOUR_MAGENTA 3   COLOUR_CYAN   5
COLOUR_BLUE  1   COLOUR_GREEN   4   COLOUR_YELLOW 6
COLOUR_RED   2                      COLOUR_WHITE  7
```

All six functions are **stateful and sticky** — they update attribute state that subsequent `print` calls will read. No per-call colour argument.

## State ownership

`ink_paper` owns the attribute state. No external helper file, no `host_caps` entanglement.

```c
/* ink_paper.h — public API */
void ink(byte c);
void paper(byte c);
void bright(byte n);
void flash(byte n);
void inverse(byte n);
void over(byte n);

/* Read accessors — used by print_at's host branch. */
unsigned int attr_fg(void);   /* host: termbox2 TB_RED | TB_BOLD etc.; ZXN: unused today */
unsigned int attr_bg(void);
```

`ink_paper.c` keeps the state as file-scope variables and DRYs the six setters through one internal helper:

```c
static void set_attr(byte kind, byte val) {
  /* ZXN: two-byte control-code sequence; host: update shadow. */
}
void ink(byte c)    { set_attr(ATTR_INK,    c); }
void paper(byte c)  { set_attr(ATTR_PAPER,  c); }
/* ... */
```

**Memory layout (future).** When the ZXN build cares about where state lives, the shadow variables get pinned to a specific address via SDCC `__at` or a linker section. No architectural change — just a decoration on the existing variables.

## ZXN implementation

`set_attr` emits a two-byte control sequence through a private `RST 10h` wrapper (file-scope scratch byte + inline asm, same pattern as `print_at.c`). The control codes are standard ZX Spectrum:

| Code | Followed by | Meaning |
|------|-------------|---------|
| `16` | `n` (0-7) | INK |
| `17` | `n` (0-7) | PAPER |
| `18` | `n` (0-1) | FLASH |
| `19` | `n` (0-1) | BRIGHT |
| `20` | `n` (0-1) | INVERSE |
| `21` | `n` (0-1) | OVER |

The ROM maintains its own attribute sysvars and applies them to all subsequent `RST 10h` output, so on ZXN the `attr_fg`/`attr_bg` accessors have no consumers today — `print_at`'s ZXN branch is unchanged, because the ROM already knows the state. We keep the accessors in the header anyway so the API is target-uniform; the ZXN implementations can be stubs or real getters depending on whether we decide to mirror state into our own shadow now or defer until the raster migration.

The `RST 10h` wrapper is **deliberately duplicated** from `print_at.c` (~8 lines). It's a ROM-era implementation coincidence that both files happen to use this vector — in the raster era both sites rewrite independently and the wrappers vanish from both. Extracting a shared `zx_putchar` now would encode the coincidence into the architecture and create work on raster day.

## Host implementation

`set_attr` updates file-scope `static` shadow variables (termbox2 `fg`, `bg`, `attr` bits). `attr_fg()` / `attr_bg()` return the composed values. `print_at.c`'s host branch calls them instead of passing `TB_DEFAULT, TB_DEFAULT` to `tb_print`.

Spectrum palette 0-7 maps directly to `TB_BLACK..TB_WHITE` (both are 3-bit RGB in the same order). Bright maps to `TB_BOLD`.

**Host capability flag.** Add `byte ink` to `rock_host_caps` in [[rtl/rtl-host-caps]], set when termbox2 successfully initialises (same rule as `print_at`). The setters no-op when the flag is 0 so the piped-stdout test harness keeps working deterministically.

## Rule 1 clarification

Under the agreed DRY rewording of [[rtl/rtl-overview]] rule 1 (components are leaves; duplication among siblings is a smell, extract when needed), `print_at.c` reading `ink_paper`'s public accessors is **not** a rule violation — it's legitimate API consumption. What rule 1 prohibits is components reaching into each other's implementation details or duplicating state that belongs elsewhere. Public header + accessor functions are the front door.

Proposed clarifying sentence to add to rule 1 when updating `rtl-overview.md`:

> A component MAY consume another component's public header for its declared API. It MUST NOT reach into another's implementation file or duplicate its state.

## Files to create / modify

| File | Action | Purpose |
|------|--------|---------|
| `src/lib/ink_paper.h` | **new** | Public API, colour `#define`s, `attr_fg`/`attr_bg` accessors. |
| `src/lib/ink_paper.c` | **new** | State vars, internal `set_attr` helper, six setters, two accessors. ZXN private `RST 10h` wrapper. |
| `src/lib/print_at.c` | edit | Host branch: `#include "ink_paper.h"`, pass `attr_fg()`/`attr_bg()` to `tb_print`. ZXN branch unchanged. |
| `src/lib/host_caps.h` | edit | Add `byte ink;` to `rock_host_caps`. |
| `src/lib/host_caps.c` | edit | Host branch: set `host_caps.ink = 1` on successful `tb_init`. |
| `src/generator.c` | edit | `register_builtin × 6` (ink/paper/bright/flash/inverse/over); `#include "ink_paper.h"` in transpile(). |
| `rock` | edit | Append `$RT/lib/ink_paper.c` to `RTL_C_SRCS`. |
| `test/ink_paper_test.rkr` | **new** | Host smoke test: set colours, call `print(x, y, text)`, assert functions return normally. |
| `wikiroot/pages/rtl/rtl-overview.md` | edit | Add the rule 1 clarifying sentence above. |
| `wikiroot/pages/rtl/rtl-ink-paper.md` | update | Flip `status: draft` → `status: current` on ship. ✅ |

## Verification plan

1. **Host tests:** `./run_tests.sh` — existing tests green; new `ink_paper_test` asserts setters return normally on piped stdout.
2. **Host manual:** run in a real terminal — termbox2 draws the test string in the chosen colour combo.
3. **ZXN build:** `./rock --target=zxn test/ink_paper_test.rkr` produces `.nex` cleanly.
4. **ZXN hardware (optional):** CSpect / real Next, verify colours on screen.

## Non-goals

- **Per-call colour arguments** (`print(x, y, text, ink, paper)`) — different API philosophy.
- **Direct attribute-memory writes** to `$5800-$5AFF` — part of the raster TODO on [[rtl/rtl-print-at]].
- **Extended palette / Layer 2** — separate future component.
- **Save/restore attributes** — speculative, omitted until a concrete need appears.

## Open questions

- **Out-of-range values** (`ink(8)`, `bright(2)`) — clamp, wrap, or pass through? Recommendation: pass through on both targets, matching the `print_at` coordinate-clipping stance.

## See Also

- [[rtl/rtl-print-at]] — host branch gets the `attr_fg`/`attr_bg` reads
- [[rtl/rtl-overview]] — rule 1 gains the public-header clarification
- [[rtl/rtl-host-caps]] — gains the `ink` capability flag
- [[rtl/rtl-border]] — adjacent colour component (border is ULA port, unrelated to character-cell attributes)
