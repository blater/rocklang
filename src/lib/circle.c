#include "circle.h"
#include "plot.h"

/* Integer midpoint circle algorithm (Bresenham variant).
 *
 * Uses signed ints for the offsets so `cx - y` and `cy - x` can go
 * negative without underflowing the byte arithmetic; the off-screen
 * pixels get filtered when we cast back to byte and plot() clips
 * y >= 192. For x we rely on the natural wrap at 256 plus the fact
 * that the ULA scanline is exactly 256 pixels wide, so any byte value
 * is a valid column — the only clipping that matters is vertical.
 *
 * Eight-way symmetry: for each (x, y) in the first octant we stamp
 * eight points by reflecting across both axes and the diagonal.
 *
 * Uses plot_nopresent per pixel and plot_flush once at the end so the
 * host termbox2 path amortises the tb_present over the whole circle.
 * On ZXN plot_flush is a no-op.
 */
static void stamp8(int cx, int cy, int x, int y) {
  int pts[8][2] = {
    {cx + x, cy + y}, {cx - x, cy + y},
    {cx + x, cy - y}, {cx - x, cy - y},
    {cx + y, cy + x}, {cx - y, cy + x},
    {cx + y, cy - x}, {cx - y, cy - x},
  };
  for (int i = 0; i < 8; i++) {
    int py = pts[i][1];
    if (py < 0 || py >= 192) continue;
    plot_nopresent((byte)pts[i][0], (byte)py);
  }
}

void circle(byte cx, byte cy, byte r) {
  if (r == 0) {
    plot(cx, cy);
    return;
  }

  int icx = (int)cx;
  int icy = (int)cy;
  int x = 0;
  int y = (int)r;
  int d = 1 - y;   /* midpoint decision variable */

  while (x <= y) {
    stamp8(icx, icy, x, y);
    x++;
    if (d < 0) {
      d += 2 * x + 1;
    } else {
      y--;
      d += 2 * (x - y) + 1;
    }
  }
  plot_flush();
}
