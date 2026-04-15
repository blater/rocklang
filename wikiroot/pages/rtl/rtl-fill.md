---
title: "RTL Component: fill"
category: concepts
tags: [rtl, graphics, fill, rectangle, raster, zxn]
sources: [src/lib/fill.c, src/lib/fill.h, src/lib/draw.c, wikiroot/pages/rtl/rtl-draw.md]
updated: 2026-04-14
status: current
---

# RTL Component: fill

Filled shapes on the ZX ULA screen. Phase 1 ships a single primitive —
`fill_rect` — implemented as a thin wrapper over [[rtl/rtl-draw]]'s
horizontal fast path. Flood fill is **deliberately deferred**; see the
non-goals section.

## Rock-facing API

```rock
fill_rect(to_byte(10), to_byte(10), to_byte(50), to_byte(40));
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `fill_rect` | `void` | `byte x0, byte y0, byte x1, byte y1` | Paint every pixel in the axis-aligned rectangle between the two endpoints (inclusive). |

- Endpoints may be given in any order; the component sorts.
- Respects the current **draw mode** — `DRAW_MODE_OR` (default) paints,
  `DRAW_MODE_XOR` toggles every covered pixel (fill twice to erase).
- Rows with `y >= 192` are skipped per-row. No horizontal clipping
  needed (the ULA scanline is 256 pixels wide = the full byte range).

## Implementation

Twenty lines of C. Normalises the endpoints, then walks y from top to
bottom calling `draw(x0, y, x1, y)` per row. Because `draw()`'s
dispatcher detects `y0 == y1` and takes the **byte-mask H fast path**,
each row costs:

- one `PIXELAD` to get the start address,
- a leading partial-byte mask,
- a tight `LD (HL), 0xFF` / `INC L` loop across the full bytes,
- a trailing partial-byte mask.

For a 128×96 fill that's ~96 PIXELADs + ~96 × 16 byte stores — the
cheapest rectangle path we can get without PIXELDN span tricks.

```c
void fill_rect(byte x0, byte y0, byte x1, byte y1) {
  if (x0 > x1) swap(x0, x1);
  if (y0 > y1) swap(y0, y1);
  byte y = y0;
  while (1) {
    draw(x0, y, x1, y);
    if (y == y1) break;
    y++;
  }
  plot_flush();
}
```

`plot_flush()` at the end amortises the host termbox2 `tb_present`
over the whole rectangle — one present per fill, not per row. On ZXN
it's a no-op.

The `while (1) { ...; if (y == y1) break; y++; }` shape (rather than
`for (y = y0; y <= y1; y++)`) is to avoid overflow when `y1 == 255`:
a `byte` counter would wrap back to 0 and loop forever. Irrelevant
today because `y` clips at 192, but cheap insurance.

## Design notes

### Why rect, not flood fill?

Flood fill is the more interesting primitive — "colour this enclosed
region" — but it needs to **read** the framebuffer pixel-by-pixel to
find boundaries, and it needs a queue or stack of scanline spans
whose worst-case depth is bounded by the image size. On ZXN the stack
lives somewhere below `$FFFF` with SDCC's sdcc_iy clib, and an
unbounded span queue is a fast way to crash a program that looks
fine on host. Shipping rect fill first gives us:

- the useful 90%-case primitive (UI panels, clear-within-clip, bar
  charts), and
- a known-bounded C implementation (stack usage = one function frame).

Flood fill will come later, probably as `flood_fill(x, y)` with a
fixed-size span queue and a graceful "queue full → abort" failure
mode. It's a separate component with its own test surface.

### Why delegate to `draw` instead of inlining the byte-mask walk?

`draw.c`'s `draw_hline` is already written, already tested, already
takes the draw-mode branch. Inlining a second copy into `fill.c`
would:

- double the asm maintenance burden,
- duplicate the mode split,
- save one C-level function call per row — negligible next to the
  PIXELAD.

Delegation is strictly better here. If a profiler later shows the
per-call overhead mattering on ZXN, we can revisit and inline — but
it won't.

### No `fill_circle`

Circle fills want a different loop shape (walk y, compute half-width
from `sqrt(r² - dy²)` using the same midpoint arithmetic, horizontal
span per y). It's ~40 lines of its own. Ships under [[rtl/rtl-circle]]'s
successor when a concrete need appears.

## Files

| File | Role |
|------|------|
| `src/lib/fill.h` | Public API: `fill_rect(x0, y0, x1, y1)`. |
| `src/lib/fill.c` | Vertical sweep that delegates each row to `draw`. |
| `src/generator.c` | `register_builtin("fill_rect", "void")`; `#include "fill.h"` in transpile. |
| `rock` | `fill.c` appended to `RTL_C_SRCS`. |
| `test/fill_rect_test.rkr` | Degenerate rect, small/tall/wide, reversed endpoints, overhang clip, XOR erase, panel + outline combo. |

## Verification

- **Host tests:** 242/242 passing (241 prior + 1 new `fill_rect_test`).
- **ZXN build:** clean `fill_rect_test.nex`; only the expected SDCC
  info 218 pixelad/setae warnings from `draw.c`.
- **Visual verification:** pending CSpect / real-hardware run.

## Non-goals

- **Flood fill (`flood_fill(x, y)`).** Needs framebuffer reads + a
  span queue. Deferred until we have a target program that needs it
  and we can pick a sensible max-queue-size.
- **Filled circle.** Separate component — different inner loop.
  Deferred.
- **Filled polygon / triangle.** Needs edge-list sort + active-edge
  table. Not a Phase 1 primitive.
- **Gradient / pattern fills.** No colour control at the pixel level
  on classic ULA (attributes are cell-level via [[rtl/rtl-ink-paper]]).
- **Clipping rectangle / state.** Callers clip at the coordinates
  they pass.

## See Also

- [[rtl/rtl-draw]] — horizontal fast path that fill_rect delegates to
- [[rtl/rtl-plot]] — pixel primitive + draw mode state
- [[rtl/rtl-circle]] — outline circle; fill sibling deferred
- [[targets/zxn/zxn-ula]] — ULA memory layout
