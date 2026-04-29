---
title: "RTL Component: triangle"
category: concepts
tags: [rtl, graphics, triangle, raster, scanline, zxn]
sources: [src/lib/triangle.c, src/lib/triangle.h, src/lib/draw.c, src/lib/fill.c, wikiroot/pages/rtl/rtl-draw.md]
updated: 2026-04-14
status: current
---

# RTL Component: triangle

Triangle outline and filled triangle on the ZX ULA screen. Both take
three `(x, y)` vertices in any order. Same framebuffer, same draw
mode, same host shadow as [[rtl-plot]] / [[rtl-draw]].

## Rock-facing API

```rock
triangle(to_byte(10), to_byte(10),
         to_byte(60), to_byte(10),
         to_byte(35), to_byte(60));

filled_triangle(to_byte(100), to_byte(20),
                to_byte(140), to_byte(20),
                to_byte(120), to_byte(80));
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `triangle`        | `void` | `byte x1, y1, x2, y2, x3, y3` | Outline: three edges between the vertices. |
| `filled_triangle` | `void` | `byte x1, y1, x2, y2, x3, y3` | Scanline-rasterised solid triangle including its outline. |

- Vertex order does not matter; `filled_triangle` sorts by y
  internally, and `triangle` is symmetric under rotation.
- Colour comes from whichever attribute cell each pixel lands in —
  set the ink via [[rtl-ink-paper]] before drawing. The framebuffer
  sees bits, not colours; "fills with current ink colour" is a
  natural consequence of the ULA attribute layout, not a special
  case in this component.
- Both routines honour the current **draw mode** from [[rtl-plot]]:
  `DRAW_MODE_OR` (default, additive) or `DRAW_MODE_XOR` (toggle —
  draw twice to erase).
- Pixels with `y >= 192` are skipped per-row inside `draw()`, so the
  outline and fill clip cleanly if vertices overhang the screen.

## Outline implementation

Three `draw()` calls — one per edge. `draw()` picks the best routine
per edge (H fast path, V fast path, or Bresenham general), so a
right-angled triangle with one horizontal and one vertical edge gets
two fast paths and one Bresenham almost for free.

## Filled implementation

Standard **scanline-rasterised triangle**:

1. **Sort** the three vertices by `y` so `y1 <= y2 <= y3` (three
   swaps, no allocation).
2. **Upper half** — walk `y` from `y1` to `y2 - 1`. For each row
   compute `xa = edge_x(y, v1→v2)` and `xb = edge_x(y, v1→v3)` and
   emit one horizontal span.
3. **Lower half** — walk `y` from `y2` to `y3` (the middle row is
   emitted here so the middle vertex always gets included). Edges
   are now `v2→v3` and `v1→v3`.
4. Each span is `draw(xl, y, xr, y)`, which takes `draw.c`'s
   byte-mask H fast path — one PIXELAD per row plus contiguous byte
   stores.

`plot_flush()` is called once at the end so the host termbox2 path
amortises `tb_present` over the whole triangle; on ZXN it's a no-op.

### Edge interpolation

For a vertex pair `(x0, y0) → (x1, y1)` and a scanline `y`, the
x-coordinate is:

```
xa = x0 + (y - y0) * (x1 - x0) / (y1 - y0)
```

**SDCC `int` is 16-bit on Z80.** A naive C translation of that
formula overflows the intermediate product: `(y - y0) * (x1 - x0)`
can reach `191 * 255 = 48705`, which is outside signed-int range.
Rather than paying for 32-bit arithmetic on every row, the
implementation does the magnitude in `unsigned` (which fits 48705 ≤
65535) and carries the sign of `x1 - x0` separately:

```c
static int edge_x(byte y, byte y0, byte y1, byte x0, byte x1) {
  if (y1 == y0) return (int)x0;        // degenerate horizontal edge
  unsigned dy       = (unsigned)(y  - y0);
  unsigned dy_total = (unsigned)(y1 - y0);
  int idx = (int)x1 - (int)x0;
  unsigned udx = (idx < 0) ? (unsigned)(-idx) : (unsigned)idx;
  unsigned q = (dy * udx) / dy_total;
  return (idx < 0) ? ((int)x0 - (int)q) : ((int)x0 + (int)q);
}
```

One 16-bit multiply and one 16-bit divide per edge per row. On the
Z80N at 28 MHz a full-screen triangle (~190 rows × 2 edges × one
divide each) is well under a millisecond — acceptable without
resorting to DDA or fixed-point tricks on first ship.

### Degenerate cases

- **All three vertices on one scanline** (`y1 == y3` after sort):
  no edges to walk. Emits a single span from the leftmost to the
  rightmost x and exits. Without this short-circuit the upper half
  runs zero iterations and the lower half calls `edge_x` with
  `y2 == y3` and `y1 == y3`, which the `y1 == y0` guard handles, but
  the explicit branch is clearer and saves work.
- **Flat-top** (`y1 == y2`): upper half is zero rows, lower half
  handles everything. No special case needed.
- **Flat-bottom** (`y2 == y3`): lower half is a single row at `y3`.
  No special case needed.
- **Coincident vertices** (point or line): fall through the flat
  cases or the all-on-one-row case.

### Why scanline instead of a seed-based fill?

A seed fill would need framebuffer reads and a bounded stack of
spans — awkward to size on the Z80N under SDCC `sdcc_iy`. Scanline
rasterisation is stackless, deterministic, and O(rows × 1 divide per
edge). See [[rtl-fill]]'s non-goals for the same reasoning applied
to rectangle fill and deferred flood fill.

## Host vs ZXN

**Single C implementation, no target split.** Both halves of the
component go through `draw()`, which already handles host-vs-ZXN
routing. On ZXN every scanline hits the PIXELAD byte-mask walk;
on host every scanline writes the shadow buffer and redraws the
affected quadrant-block cells, with a single `tb_present` at the end.

## Files

| File | Role |
|------|------|
| `src/lib/triangle.h` | Public API: `triangle`, `filled_triangle`. |
| `src/lib/triangle.c` | Outline + scanline-rasterised fill + edge interp helper. |
| `src/generator.c` | `register_builtin` for both names; `#include "triangle.h"` in transpile. |
| `rock` | `triangle.c` appended to `RTL_C_SRCS`. |
| `test/triangle_test.rkr` | Outline + fill cases: flat-top, flat-bottom, pointy-up, pointy-down, collinear, bottom-overhang, XOR erase, full-screen. |

## Verification

- **Host tests:** 243/243 passing (242 prior + 1 new `triangle_test`).
- **ZXN build:** clean `triangle_test.nex`; only the expected SDCC
  info 218 pixelad/setae warnings from `draw.c`.
- **Visual verification:** pending CSpect / real-hardware run.

## Non-goals

- **Anti-aliasing.** Pixels are 1-bit; AA would require per-cell
  coverage tracking against the attribute grid. No.
- **Barycentric shading / per-vertex colour.** Colour is cell-level
  on the ULA — wrong layer to shade.
- **Triangle strip / fan APIs.** Compose at the Rock call site if
  you need them; `triangle`/`filled_triangle` are per-triangle.
- **Hand-rolled asm inner loop.** The scanline walk delegates to
  `draw()`'s already-tuned H fast path, so there's little to gain
  from a dedicated asm rewrite. Revisit if profiling proves
  otherwise.
- **DDA / fixed-point edge walking.** The naive one-divide-per-edge
  per-row approach is cheap enough on the Z80N. A 16.16 fixed-point
  DDA would be faster for tall thin triangles but adds state and
  failure modes. Deferred.

## See Also

- [[rtl-draw]] — line primitive used for each edge and each fill span
- [[rtl-fill]] — same "walk y, call draw per row" pattern for rects
- [[rtl-plot]] — pixel primitive underlying everything
- [[rtl-ink-paper]] — attribute cell colour control
