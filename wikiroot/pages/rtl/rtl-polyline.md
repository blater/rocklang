---
title: "RTL Component: polyline"
category: concepts
tags: [rtl, graphics, line, raster, zxn]
sources: [src/lib/polyline.c, src/lib/polyline.h, src/lib/draw.c, src/lib/fundefs_internal.c]
updated: 2026-04-14
status: current
---

# RTL Component: polyline

Thin wrapper over [[rtl/rtl-draw]]. `polyline(xs, ys)` takes two parallel
`byte[]` arrays of X and Y coordinates and draws N-1 connected line
segments between consecutive points. No new geometry code — it just
walks the arrays and delegates each segment to `draw()`.

## Rock-facing API

```rock
byte[] xs := [];
byte[] ys := [];
append(xs, to_byte(10));  append(ys, to_byte(10));
append(xs, to_byte(50));  append(ys, to_byte(10));
append(xs, to_byte(90));  append(ys, to_byte(40));
polyline(xs, ys);  // two segments: (10,10)→(50,10)→(90,40)
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `polyline` | `void` | `byte[] xs, byte[] ys` | Draw connected segments between paired points. |

- Inherits all of `draw`'s behaviour: H/V dispatcher fast paths,
  Bresenham general fallback, per-pixel y-clip, current draw mode
  (OR / XOR).
- **Length < 2** → no-op. A single point is not drawn (use `plot` for
  that).
- **Length mismatch** → clamped to the shorter of the two arrays. The
  tail of the longer one is silently discarded. This matches the
  general RTL stance of producing partial output rather than reading
  past the end.
- Closing a polyline (triangle, rectangle outline) is the caller's
  responsibility: append the first point again at the end.

## Implementation

Twenty lines of C. Reads `xs->length` and `ys->length` directly off
the `__internal_dynamic_array_t` struct, picks the minimum, and uses
`byte_get_elem` (bounds-checked element accessor from
`fundefs_internal.c`) inside the loop. Keeps a rolling `(x0, y0)` and
calls `draw(x0, y0, x1, y1)` per step.

```c
void polyline(__internal_dynamic_array_t xs, __internal_dynamic_array_t ys) {
  if (!xs || !ys) return;
  size_t n = xs->length < ys->length ? xs->length : ys->length;
  if (n < 2) return;
  byte x0 = byte_get_elem(xs, 0);
  byte y0 = byte_get_elem(ys, 0);
  for (size_t i = 1; i < n; i++) {
    byte x1 = byte_get_elem(xs, i);
    byte y1 = byte_get_elem(ys, i);
    draw(x0, y0, x1, y1);
    x0 = x1;
    y0 = y1;
  }
}
```

Both targets share the same implementation — there's nothing
target-specific to do because `draw` already handles all the ZXN vs
host split below. That's the point of a wrapper: zero duplicated
logic.

## Design notes

### Why parallel arrays instead of a `point[]` record array?

Rock doesn't yet have a stable story for arrays of user-defined
records passed across the C ABI, and even if it did, the caller would
have to define a `point` record before every program that wants to
draw. Parallel `byte[]` arrays are the lowest-friction API available
today. If/when a shipped `point` record gains traction we can add a
second overload.

### Why clamp mismatched lengths instead of erroring?

The component is a graphics primitive — the blast radius of a
mismatch is "some pixels you didn't intend". Partial output is more
useful than a crash or a Rock-level exception (of which there are
none). The clamp also guarantees every `byte_get_elem` call stays
in-bounds, so the bounds-check assertion inside the RTL never fires.

### Why not also ship `polygon` (auto-closed)?

Trivially: `append(xs, xs[0]); append(ys, ys[0]); polyline(xs, ys);`.
The caller already has a mutable array in hand. Adding a `polygon`
entry point would be two lines of C and one more builtin registration
— reasonable, but no concrete need yet.

## Files

| File | Role |
|------|------|
| `src/lib/polyline.h` | Public API: `polyline(xs, ys)`. |
| `src/lib/polyline.c` | 20-line wrapper over `draw`. |
| `src/generator.c` | `register_builtin("polyline", "void")`; `#include "polyline.h"` in transpile. |
| `rock` | `polyline.c` appended to `RTL_C_SRCS`. |
| `test/polyline_test.rkr` | Zig-zag, triangle outline, length-0/1/mismatch edge cases, XOR mode. |

## Verification

- **Host tests:** 240/240 passing (239 prior + 1 new `polyline_test`).
- **ZXN build:** `./rock --target=zxn test/polyline_test.rkr` produces
  a clean `polyline_test.nex`. SDCC info 218 warnings for
  `pixelad`/`setae` come from `draw.c`, not polyline itself.
- **ZXN hardware:** CSpect / real Next verification pending — shared
  with `draw`'s pending visual check.

## See Also

- [[rtl/rtl-draw]] — the line primitive polyline delegates to
- [[rtl/rtl-plot]] — the pixel primitive underpinning both
- [[concepts/array-internals]] — `__internal_dynamic_array_t` layout
- [[rtl/rtl-overview]] — RTL component conventions
