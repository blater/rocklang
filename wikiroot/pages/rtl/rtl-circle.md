---
title: "RTL Component: circle"
category: concepts
tags: [rtl, graphics, circle, raster, zxn, bresenham]
sources: [src/lib/circle.c, src/lib/circle.h, src/lib/plot.c, wikiroot/pages/rtl/rtl-plot.md, wikiroot/pages/rtl/rtl-draw.md]
updated: 2026-04-14
status: current
---

# RTL Component: circle

Raster circle outline on the ZX ULA screen using the integer midpoint
algorithm. Sibling to [[rtl/rtl-plot]] and [[rtl/rtl-draw]] — same framebuffer,
same draw mode, same host shadow. Outline only — no fill yet.

## Rock-facing API

```rock
circle(to_byte(128), to_byte(96), to_byte(40));  // big circle, centred
circle(to_byte(50),  to_byte(50), to_byte(1));   // tiny — 4 px
circle(to_byte(10),  to_byte(10), to_byte(0));   // r=0 → single plot
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `circle` | `void` | `byte cx, byte cy, byte r` | Draw the outline of a circle centred at (cx, cy) with radius r. |

- Centre `(cx, cy)` in the same top-left pixel grid as `plot` / `draw`.
- `r == 0` degenerates to a single `plot` call.
- Pixels with `y >= 192` are clipped individually; pixels with negative
  y (when `cy - y` underflows for a tall circle) are filtered in C
  before hitting `plot`. Horizontal clipping relies on the byte-width
  ULA row — any byte x is a valid column.
- Honours the sticky global **draw mode** from [[rtl/rtl-plot]]:
  `DRAW_MODE_OR` (default) or `DRAW_MODE_XOR` (toggle, erase by
  redrawing).

## Algorithm

Classic integer midpoint (Bresenham-style) circle. One octant is
walked from `(0, r)` down to the 45° diagonal where `x == y`; each
iteration stamps eight mirrored pixels. The decision variable `d`
starts at `1 - r` and picks between stepping east (`d += 2x + 1`) or
stepping south-east (`d += 2(x - y) + 1`, plus `y--`). No
trigonometry, no floating point, no square roots — just integer adds.

Total pixels drawn: ~`2*pi*r` but only `~r/sqrt(2)` loop iterations
because of the 8-way symmetry.

```c
int x = 0, y = r;
int d = 1 - r;
while (x <= y) {
  stamp8(cx, cy, x, y);   // 8 plot_nopresent calls
  x++;
  if (d < 0)  d += 2*x + 1;
  else       { y--; d += 2*(x - y) + 1; }
}
plot_flush();
```

## Host vs ZXN

**No target-specific code.** The whole component is plain C that goes
through [[rtl/rtl-plot]]'s `plot_nopresent` + `plot_flush` API. On ZXN
every pixel takes the PIXELAD + SETAE + OR/XOR inline-asm path inside
`plot()`; on host the shadow + quadrant-block redraw handles it.
Batching via `plot_flush` means the host calls `tb_present` exactly
once per circle, which is the difference between "fine" and
"stuttering" for large circles.

This is slower than a fully hand-tuned ZXN routine would be — a
dedicated `circle_zxn` could cache PIXELAD for the horizontal sweeps
and use `SETAE` only when x changes — but the component is correct
on first ship, tiny, debuggable, and identical between targets. Pure
perf work is deferred until a real program needs it.

## Design notes

### Why outline-only?

Filled circles share almost nothing with the outline algorithm — they
want a scanline sweep, not the 8-way stamping. When a fill primitive
ships ([[rtl/rtl-overview]] roadmap) it will likely expose
`filled_circle(cx, cy, r)` as a sibling rather than an argument on
this function.

### Signed-int offsets instead of byte math

The straightforward implementation uses `byte` throughout and relies
on unsigned wrap for `cx - y`. That works, but it also silently
wraps pixels from the far side of the screen back into the frame on
tall or off-centre circles, painting phantom dots. Promoting offsets
to `int` in C lets us reject negative y explicitly before plot — a
single comparison per stamped point. Horizontal offsets still wrap
because the ULA scanline is exactly 256 pixels wide, so any byte
value is a valid column.

### stamp8 uses a local array

Eight points worth of `int[2]` lives on the stack per call. Cheap on
host, cheap on ZXN (SDCC emits sensible stack frames for small local
arrays). The alternative is eight inline `plot_nopresent` calls —
shorter code, but messier to read and no measurable win.

## Files

| File | Role |
|------|------|
| `src/lib/circle.h` | Public API: `circle(cx, cy, r)`. |
| `src/lib/circle.c` | Midpoint algorithm + `stamp8` helper. Both targets. |
| `src/generator.c` | `register_builtin("circle", "void")`; `#include "circle.h"` in transpile. |
| `rock` | `circle.c` appended to `RTL_C_SRCS`. |
| `test/circle_test.rkr` | r=0, r=1, small/large radii, y-clip on both edges, XOR erase, combined with `draw`. |

## Verification

- **Host tests:** 241/241 passing (240 prior + 1 new `circle_test`).
- **ZXN build:** clean `.nex`; only the usual `info 218 pixelad/setae`
  SDCC warnings from `draw.c`.
- **Visual verification:** pending CSpect / real-hardware run.

## Non-goals

- **Filled circle.** Separate component — scanline approach, see fill.
- **Ellipse.** Would need a second decision-variable pair and far more
  test coverage. No concrete need yet.
- **Arc / wedge / pie.** Would need angular bookkeeping. Deferred.
- **Hand-rolled asm inner loop.** Would use cached PIXELAD + column
  increments along each scanline of the outline. Measurable perf win,
  large code-size cost, not on the critical path.

## See Also

- [[rtl/rtl-plot]] — pixel primitive; `circle` delegates to `plot_nopresent`
- [[rtl/rtl-draw]] — line primitive; shares the same draw-mode state
- [[rtl/rtl-polyline]] — connected segments; same shape-wrapping idea
- [[targets/zxn/zxn-ula]] — ULA memory layout used by the underlying plot
