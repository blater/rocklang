---
title: "RTL Component: plot"
category: concepts
tags: [rtl, graphics, pixel, raster, zxn]
sources: [src/lib/plot.c, src/lib/plot.h, src/lib/print_at.c, src/lib/ink_paper.c, wikiroot/pages/targets/zxn/zxn-ula.md, wikiroot/pages/rtl/rtl-overview.md]
updated: 2026-04-14
status: current
---

# RTL Component: plot

First raster-graphics RTL component. Sets a single pixel on the classic
ZX ULA display. Deliberately minimal — this is the foundation that
later `draw_line`, `circle`, and the raster rewrite of `print_at` will
build on.

**Status:** shipped. 234/234 host tests pass; `plot_test.nex` builds clean on ZXN.

## Rock-facing API

```rock
plot(to_byte(128), to_byte(96));   // centre pixel on 256x192
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `plot` | `void` | `byte x, byte y` | Set the pixel at (x, y) on the ULA screen. |

- Origin `(0, 0)` is **top-left** (matches `print_at`, matches
  termbox2, does **not** match Sinclair BASIC's bottom-left origin —
  we pick screen-natural over ROM-legacy).
- `x` range 0..255, `y` range 0..191. Out-of-range y is clipped
  silently (no-op) to stay consistent with `print_at`'s coordinate
  stance. x can't be out of range because it's a byte and the screen
  is 256 wide.
- No colour argument. Pixels inherit the attribute cell they land in,
  whose ink/paper comes from the current [[rtl-ink-paper]] state.
- `plot` honours the sticky global **draw mode** — `DRAW_MODE_OR`
  (default) merges additively, `DRAW_MODE_XOR` toggles the pixel so
  two plots at the same coords cancel. Set via
  `set_draw_mode(to_byte(0|1))`. Also respected by [[rtl-draw]].
  XOR covers the erasable-cursor idiom; a dedicated ERASE mode was
  considered and rejected as redundant.

## ZXN implementation

The Next has two Z80N instructions that do all the hard work in 16
T-states combined (see [[targets/zxn/zxn-ula]]):

- `PIXELAD` — D=Y, E=X → HL = address of the pixel's byte in screen
  memory (encodes the non-linear ULA address math)
- `SETAE` — E=X → A = bitmask `0x80 >> (x & 7)` for the pixel within
  its byte

So the core is literally four instructions:

```asm
; D = y, E = x on entry
PIXELAD          ; HL = &screen[y][x>>3]
SETAE            ; A  = pixel bitmask
OR   (HL)        ; merge with existing byte
LD   (HL), A
```

Because we set a single bit (OR, not XOR or AND), the existing
contents of the byte are preserved — plotting is **additive**. This
matches what most Rock programs will want as the default; erase and
toggle variants can wrap the same address math later.

**Calling convention.** `plot(byte x, byte y)` with SDCC `sdcc_iy`
passes args on the stack. Rather than fight the calling convention
inline, the C entry point loads x and y into file-scope scratch bytes
and calls a naked inline-asm routine that reads them with absolute
`LD A,(nn)` — exactly the same pattern `print_at.c` uses for
`rock_zx_rom_byte`. This keeps the asm short, register-hygiene
trivial, and independent of any SDCC calling-convention quirks.

```c
/* plot.c (ZXN branch) */
unsigned char rock_plot_x;
unsigned char rock_plot_y;

static void plot_raw(void) {
  __asm
    ld a, (_rock_plot_y)
    ld d, a
    ld a, (_rock_plot_x)
    ld e, a
    pixelad             ; HL = address
    setae               ; A  = mask
    or (hl)
    ld (hl), a
  __endasm;
}

void plot(byte x, byte y) {
  if (y >= 192) return;
  rock_plot_x = x;
  rock_plot_y = y;
  plot_raw();
}
```

**No ROM dependency.** Unlike `print_at`, this routine never touches
`RST 10h` — it writes the ULA framebuffer directly at `$4000-$57FF`.
That means `plot` keeps working after the ROM is paged out, which is
the whole point of the raster migration tracked on [[rtl-print-at]].

**Z80N requirement.** `PIXELAD` and `SETAE` are Next-only
instructions. Plain Z80 targets (48K, 128K, +3) would need the
classic bit-permutation math — out of scope for Phase 1. z80asm
`-mz80n` is already implied by `+zxn`, so no build flag changes.

## Host implementation

The host branch is a best-effort visualisation, not a real
framebuffer. termbox2 is a character-cell terminal library — we have
no access to sub-character pixels. Two viable approaches:

1. **Quadrant block characters** (`▖▗▘▝▀▄▌▐█`). Each terminal cell
   represents a 2×2 pixel block; we keep a 256×192 bit shadow in
   memory and redraw the affected cell on each `plot` call. Gives a
   128×96 visible resolution — coarse but recognisable.
2. **Plain-text log** (piped-stdout fallback). `printf("plot(%u,%u)\n", x, y)`.
   Only for the test harness; matches `print_at`'s fallback stance.

Phase 1 ships **both**, gated on `host_caps.plot` the same way
`print_at` is gated on `host_caps.print_at`. termbox2 init is already
done in `host_caps.c`; `plot` reuses it and sets its own capability
flag once.

**Shadow buffer.** `static unsigned char plot_shadow[192][32];`
(6144 bytes — the same size as the ULA bitmap). File-scope in
`plot.c`. Not exposed to other components.

**Redraw granularity.** On each `plot` call, mark the affected 2x2
quadrant, recompute the glyph for that cell, call `tb_set_cell` +
`tb_present`. `tb_present` is cheap enough for a few thousand
plots/sec which is all a test harness needs.

**Colour.** Pass `attr_fg()`/`attr_bg()` from [[rtl-ink-paper]], same
as `print_at` host. Attribute cells are 8×8 pixels on the real
machine and 2×2 on the host shadow — we accept the mismatch. A pixel
plotted "in red" on the host shows red in its 2x2 cell; on ZXN it
colours the 8×8 attribute cell it lands in. Not identical, but close
enough for development.

## Files to create / modify

| File | Action | Purpose |
|------|--------|---------|
| `src/lib/plot.h` | **new** | Public API. |
| `src/lib/plot.c` | **new** | ZXN inline-asm wrapper; host shadow + termbox2 redraw; plain-text fallback. |
| `src/lib/host_caps.h` | edit | Add `byte plot;` to `rock_host_caps`. |
| `src/lib/host_caps.c` | edit | Host: set `host_caps.plot = 1` on successful `tb_init` (reuses existing init — no second probe). |
| `src/generator.c` | edit | `register_builtin("plot", 2, VOID, BYTE, BYTE)`; `#include "plot.h"` in `transpile()`. |
| `rock` | edit | Append `$RT/lib/plot.c` to `RTL_C_SRCS`. |
| `test/plot_test.rkr` | **new** | Calls `plot` at a handful of coordinates incl. boundaries; asserts it returns. |
| `wikiroot/pages/rtl/rtl-plot.md` | update | Flip `status: draft` → `status: current` on ship. |
| `wikiroot/pages/rtl/rtl-overview.md` | edit | Add `plot` to component list. |
| `wikiroot/pages/rtl/rtl-print-at.md` | edit | Cross-reference — `plot` is the first step of the raster migration. |

## Verification plan

1. **Host tests:** `./run_tests.sh` — existing 230 tests green;
   new `plot_test.rkr` passes via the piped-stdout fallback.
2. **Host manual:** run `plot_test` in a real terminal — verify
   quadrant-block rendering looks right for a few simple shapes
   (horizontal line, vertical line, diagonal).
3. **ZXN build:** `./rock --target=zxn test/plot_test.rkr` produces a
   clean `.nex`.
4. **ZXN hardware:** CSpect / real Next — plot a recognisable shape
   (diagonal line, box) and eyeball it. No automated ZXN assertion
   infra yet.

## Non-goals (Phase 1)

- **Lines, circles, fills** — separate components once `plot` is
  proven. Each gets its own page and its own ship.
- **Plain Z80 targets** — relies on Z80N `PIXELAD`/`SETAE`.
- **Unplot, toggle, XOR modes** — add when a concrete need appears.
- **Attribute writes** — plot writes bitmap only. Attribute cells
  stay whatever they were (usually the ROM default set by CLS or the
  current ink/paper from a prior `print`). A future `plot_attr` or
  extension to `ink_paper` can address this.
- **Layer 2 / sprite layers** — this is ULA-only. Layer 2 is a
  separate linear framebuffer with its own component.
- **Raster `print_at` migration** — shares the framebuffer knowledge
  but is a bigger, separate piece of work. `plot` unblocks it by
  proving the ZXN direct-write pattern end-to-end.

## Open questions

- **Shadow buffer size on host.** 6144 bytes is fine; mentioning for
  completeness. Not a concern.
- **Quadrant-glyph lookup table.** 16 entries (4 bits → 1 of
  `' ▘▝▀▖▌▞▛▗▚▐▜▄▙▟█'`). Keep inline in `plot.c`.
- **Should `cls` (planned) also zero the shadow buffer?** Yes, but
  `cls` isn't written yet — note for the `cls` component's design.

## See Also

- [[targets/zxn/zxn-ula]] — ULA memory layout, `PIXELAD`/`SETAE` reference
- [[rtl-print-at]] — shares the future raster path
- [[rtl-ink-paper]] — attribute source for host-side colouring
- [[rtl-host-caps]] — gains the `plot` capability flag
- [[rtl-overview]] — component conventions
