#include "polyline.h"
#include "draw.h"
#include "fundefs_internal.h"

/* Walk the two arrays in lock-step and emit a draw() per segment. Length
 * is clamped to the shorter of the two so a caller that accidentally
 * passes mismatched arrays gets partial output instead of a read past
 * the end. */
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
