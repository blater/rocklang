#include "fill.h"
#include "draw.h"
#include "plot.h"

/* Vertical sweep of horizontal lines. Delegates each row to draw() with
 * y0 == y1, which takes draw.c's byte-mask H fast path — one PIXELAD per
 * row and contiguous byte stores across the span. The per-row y >= 192
 * check inside draw() handles clipping, so we only need to sort the
 * endpoints and walk.
 *
 * plot_flush is called once at the end so the host termbox2 path
 * amortises tb_present across the whole rectangle; on ZXN it's a no-op.
 */
void fill(byte x0, byte y0, byte x1, byte y1) {
  if (x0 > x1) { byte t = x0; x0 = x1; x1 = t; }
  if (y0 > y1) { byte t = y0; y0 = y1; y1 = t; }

  byte y = y0;
  while (1) {
    draw(x0, y, x1, y);
    if (y == y1) break;
    y++;
  }
  plot_flush();
}
