---
title: "RTL Component: draw"
category: concepts
tags: [rtl, graphics, line, raster, zxn, bresenham]
sources: [src/lib/draw.c, src/lib/draw.h, src/lib/plot.c, wikiroot/pages/targets/zxn/zxn-ula.md, wikiroot/pages/rtl/rtl-plot.md]
updated: 2026-04-14
status: current
---

# RTL Component: draw

Second raster-graphics RTL component. `draw(x0, y0, x1, y1)` draws a
straight line between two absolute pixel coordinates on the ZX ULA
screen. Sibling to [[rtl/rtl-plot]] — same framebuffer, same host shadow,
same `host_caps.plot` capability flag.

## Rock-facing API

```rock
draw(to_byte(0),   to_byte(0),   to_byte(255), to_byte(191));  // diagonal
draw(to_byte(0),   to_byte(96),  to_byte(255), to_byte(96));   // horizontal
draw(to_byte(128), to_byte(0),   to_byte(128), to_byte(191));  // vertical
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `draw` | `void` | `byte x0, byte y0, byte x1, byte y1` | Set all pixels along the straight line from (x0,y0) to (x1,y1). |

- Origin `(0, 0)` is **top-left**, matching [[rtl/rtl-plot]].
- Line drawing respects the global **draw mode** (see [[rtl/rtl-plot]]):
  `DRAW_MODE_OR` (default, additive) or `DRAW_MODE_XOR` (toggle —
  draw twice to erase). Set via `set_draw_mode(1)` / `set_draw_mode(0)`.
- Pixels with `y >= 192` are skipped individually — no whole-line
  clipping is done. Send valid y on both endpoints to guarantee every
  intended pixel is drawn.
- Endpoint ordering does not matter; the dispatcher normalises.

## Dispatcher

`draw()` chooses the fastest routine for the shape of the line:

| Condition | Routine | Cost profile |
|-----------|---------|--------------|
| `x0 == x1 && y0 == y1` | single `plot()` | 1 pixel write |
| `y0 == y1` | `draw_hline` — byte-mask walk | O(bytes), ~1 byte-write per 8 pixels |
| `x0 == x1` | `draw_vline` — PIXELDN loop | O(pixels), ~6 ops/pixel |
| otherwise | `draw_line_general` — Bresenham | O(pixels), ~12 ops/pixel |

Horizontal and vertical are by far the most common line shapes in
practice (UI frames, axis grids, bar graphs, clip rectangles, text
underlines) so the dispatcher's two fast paths dominate real-world
cost.

## ZXN implementation

### Horizontal line — byte-mask walk

One `PIXELAD` to get the starting address, then a contiguous byte walk
across the scanline. The ULA's non-linear address formula is constant
within a single row, so `INC L` advances to the next byte without any
further PIXELAD calls. Three regions:

1. **Leading partial byte.** `0xff >> (x0 & 7)` — bits from x0 to the
   end of its byte. OR into the first byte.
2. **Full bytes between.** `LD (HL), 0xff` — OR-ing any byte with all
   ones is the same as storing 0xff, so we skip the read.
3. **Trailing partial byte.** `0xff << (7 - (x1 & 7))` — bits from the
   start of its byte up to x1. OR into the last byte.

Same-byte special case: when the whole span fits within one byte, the
mask is `lead & trail` and we write one byte.

Row-boundary overflow is not a concern: `x` is a byte (0..255 → byte
columns 0..31), and the 32-byte ULA scanline is contiguous in memory,
so `INC L` stays within the row as long as x1 <= 255. Guaranteed.

The byte walk is written in C with a small `__asm` helper that does
the PIXELAD address compute and stores the result into a file-scope
`unsigned int rock_draw_hl`. C code then casts to `unsigned char *`
and walks byte by byte — SDCC emits `INC L` for byte-pointer increments
on Z80, which is what we want.

### Vertical line — PIXELDN loop with cached mask

One `PIXELAD` + one `SETAE` at the top, saving the pixel mask in `B`.
Then a tight loop:

```asm
  ld a, (_rock_draw_count)
  ld c, a
00001$:
  ld a, (hl)
  or b
  ld (hl), a
  pixeldn       ; HL += stride to next scanline, same x
  dec c
  jr nz, 00001$
```

Six instructions per pixel, most of them 4-8 T-states. `PIXELDN`
encodes the non-linear ULA scanline stride math in one Z80N
instruction so we pay zero ULA-math cost per row. The mask never
changes because x is fixed, so we never re-run `SETAE` inside the
loop.

### General lines — Bresenham with inlined pixel write

The classic integer midpoint algorithm, split shallow / steep on the
major axis:

- **Shallow** (`dx >= dy`): step x every iteration, step y when the
  error accumulator crosses zero.
- **Steep** (`dy > dx`): step y every iteration, step x when the error
  accumulator crosses zero.

Error initialisation is `dx >> 1` (or `dy >> 1`) for symmetric bias.
Step directions are signed (`sx`, `sy` = ±1) derived from endpoint
order, so reversed-direction lines work without pre-sorting endpoints.

The per-pixel write is a literal `__asm` block inside the C loop body
— **not** a `call plot()`. SDCC inlines it verbatim so each iteration
costs six Z80 instructions:

```
ld a, (_rock_draw_y)   ; scratch load
ld d, a
ld a, (_rock_draw_x)   ; scratch load
ld e, a
pixelad                ; HL = address (8 T-states)
setae                  ; A  = mask    (8 T-states)
or (hl)                ; merge with framebuffer
ld (hl), a
```

No function-call stack ceremony, no register save/restore — the inline
block is visible to SDCC's register allocator and it threads C state
around it correctly.

The `y >= 192` check is a cheap `(unsigned)y < 192` guard in C, run
once per pixel. Full whole-line clipping is a Phase 2 optimisation.

## Host implementation

Much simpler. A single C Bresenham loop (same shallow/steep split, no
dispatcher) calls `plot_nopresent(x, y)` per pixel, and `draw()` calls
`plot_flush()` once at the end. Rendering reuses [[rtl/rtl-plot]]'s 256×192
shadow buffer and termbox2 quadrant-block cell redraw. Batching the
`tb_present` to once per line (rather than once per pixel) is the
difference between "slow but fine" and "visibly stuttering" for long
lines.

When the host capability is disabled (piped stdout under the test
harness), `plot_nopresent` falls back to `printf("plot(%u,%u)\n", ...)`
per pixel and `plot_flush` is a no-op. The test harness sees N log
lines per draw call, which is deterministic and cheap.

### Why no dispatcher on host?

On ZXN the H/V fast paths are ~2-4× faster than the general Bresenham
because they amortise PIXELAD/SETAE across many pixels. On host those
asm tricks are irrelevant — the cost of a draw is completely dominated
by termbox2 cell redraws, which all routines would perform equally.
Adding a dispatcher would just duplicate code without speeding anything
up.

## Draw modes

Both `plot` and `draw` honour a sticky global `rock_draw_mode` byte
(declared in [[rtl/rtl-plot]]). Two modes ship:

| Constant | Value | Semantics |
|----------|-------|-----------|
| `DRAW_MODE_OR` | 0 | `new = old \| mask` — additive (default) |
| `DRAW_MODE_XOR` | 1 | `new = old ^ mask` — toggle (draw twice to erase) |

Rock-facing API:

```rock
set_draw_mode(to_byte(1));  // XOR
draw(...);                   // lay down
draw(...);                   // same args → erased
set_draw_mode(to_byte(0));  // back to OR
```

XOR covers the common erasable-cursor / moving-sprite idiom without
needing a dedicated ERASE mode. The ZXN per-pixel cost is identical
for both modes (one `or (hl)` vs one `xor (hl)`).

Implementation choice: `draw_hline` and `draw_vline` branch on the
mode once at the top (C-level `if` for hline, two full `__asm` blocks
for vline — the hot loop must not contain a mode test). The Bresenham
general path puts the mode branch around the inlined per-pixel write,
accepting ~50% overhead on that path in exchange for half the code
size. Shipping a second copy of the entire Bresenham inner loop was
considered and rejected — general lines are rarer than H/V in UI
code, and the dispatcher's fast paths already dominate real workloads.

## State sharing with plot

`draw.c` is a separate translation unit from `plot.c`, but the two
share state through `plot.h`'s public API:

- `plot_nopresent(x, y)` — write a pixel without calling `tb_present`
- `plot_flush()` — call `tb_present` once (no-op on ZXN)

`plot()` is now defined in terms of these two (`nopresent` + `flush`),
so a standalone `plot()` call still presents immediately. The split is
invisible to Rock programs.

The ZXN branch of `draw.c` does **not** use `plot_nopresent` — each
pixel is an inline `__asm` block that reads `draw.c`'s own scratch
bytes. That keeps the Bresenham inner loop free of function-call
overhead. Host goes through `plot_nopresent` because the call cost is
negligible compared to terminal redraw.

## Files

| File | Role |
|------|------|
| `src/lib/draw.h` | Public API: `draw(x0, y0, x1, y1)`. |
| `src/lib/draw.c` | Dispatcher + hline/vline fast paths + shallow/steep Bresenham. Both targets. |
| `src/lib/plot.h` | `plot_nopresent` / `plot_flush` exposed for batching. |
| `src/lib/plot.c` | `plot` defined via `plot_nopresent` + `plot_flush` on host. |
| `src/generator.c` | `register_builtin("draw", "void")`; `#include "draw.h"` in transpile. |
| `rock` | `draw.c` appended to `RTL_C_SRCS`. |
| `test/draw_test.rkr` | Exercises every dispatcher branch + shallow/steep + reversed-direction + y-clip + box outline. |

## Verification

- **Host tests:** 235/235 passing (234 prior + 1 new `draw_test`).
- **Host manual:** run `draw_test` in a real terminal — box outline,
  diagonal, H/V lines all visible as quadrant-block glyphs.
- **ZXN build:** `./rock --target=zxn test/draw_test.rkr` produces a
  clean `draw_test.nex`. SDCC emits `info 218` messages for `pixelad`
  / `setae` because its code-size estimator doesn't recognise Z80N
  mnemonics; the assembler (zcc / z80asm -mz80n) handles them
  correctly and the `.nex` is well-formed.
- **ZXN hardware:** CSpect / real Next verification pending — no
  automated ZXN assertion infra yet.

## Non-goals (Phase 1)

- **Hand-rolled asm Bresenham.** Deferred. The C Bresenham + inlined
  per-pixel `__asm` is ~2× slower than a fully asm-unrolled inner loop
  would be, but it's correct on first ship and debuggable. Pure perf
  work for Phase 2.
- **45° diagonal fast path.** Skipped. Bresenham at dx==dy is already
  optimal (the error branch is taken every iteration — cheap). Savings
  from a dedicated routine are marginal and would cost another
  dispatcher arm + another test case.
- **Off-screen line clipping.** Phase 1 skips individual out-of-range
  pixels inside the loop (`y >= 192` check per pixel). Phase 2 should
  clip the line at the y=191 boundary before Bresenham runs, to save
  the iterations entirely.
- **Circle, arc, polyline, fill.** Separate future components.
- **Plain Z80 targets.** Relies on Z80N `PIXELAD`, `SETAE`, `PIXELDN`.

## See Also

- [[rtl/rtl-plot]] — pixel primitive; `draw` shares its shadow/flush model
- [[targets/zxn/zxn-ula]] — ULA memory layout, `PIXELAD` / `SETAE` / `PIXELDN`
- [[rtl/rtl-overview]] — RTL component conventions
- [[rtl/rtl-host-caps]] — host capability flag reused from `plot`
