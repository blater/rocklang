#include "triangle.h"
#include "draw.h"
#include "plot.h"

/* -------- outline --------------------------------------------------- */

void triangle(byte x1, byte y1, byte x2, byte y2, byte x3, byte y3) {
  draw(x1, y1, x2, y2);
  draw(x2, y2, x3, y3);
  draw(x3, y3, x1, y1);
}

/* -------- filled ---------------------------------------------------- */

/* Linearly interpolate the x coordinate of a triangle edge at row y.
 *
 * Given an edge from (x0, y0) to (x1, y1) with y0 <= y <= y1, returns
 * the x value along that edge at scanline y. Callers have already
 * ensured y0 <= y1 and y0 <= y <= y1 by sorting vertices and splitting
 * the triangle at the middle vertex.
 *
 * SDCC quirk: int is 16-bit on Z80, so `(y-y0) * (x1-x0)` can reach
 * 191 * 255 = 48705 and overflow a signed int. We do the magnitude in
 * unsigned (max 48705, fits in uint16) and carry the sign separately.
 */
static int edge_x(byte y, byte y0, byte y1, byte x0, byte x1) {
  if (y1 == y0) return (int)x0;
  unsigned dy       = (unsigned)(y  - y0);
  unsigned dy_total = (unsigned)(y1 - y0);
  int idx = (int)x1 - (int)x0;
  unsigned udx = (idx < 0) ? (unsigned)(-idx) : (unsigned)idx;
  unsigned q = (dy * udx) / dy_total;
  return (idx < 0) ? ((int)x0 - (int)q) : ((int)x0 + (int)q);
}

/* Clamp a signed x into the 0..255 byte range. The edge interpolator
 * can't actually return out-of-range values for byte inputs, but the
 * sort/split logic is safer with the clamp in place. */
static byte clamp_x(int x) {
  if (x < 0)   return 0;
  if (x > 255) return 255;
  return (byte)x;
}

/* Emit one horizontal span of the filled triangle at scanline y. The
 * caller passes signed x endpoints; we sort them and delegate to
 * draw() which will take the H fast path. y clipping happens inside
 * draw() per-row. */
static void span(byte y, int xa, int xb) {
  if (xa > xb) { int t = xa; xa = xb; xb = t; }
  draw(clamp_x(xa), y, clamp_x(xb), y);
}

/* Sort three (x, y) vertices so that ya <= yb <= yc on return. Tiny
 * three-compare sort — no helper bookkeeping. */
static void sort_by_y(byte *xa, byte *ya, byte *xb, byte *yb, byte *xc, byte *yc) {
  byte tx, ty;
  if (*ya > *yb) { tx = *xa; *xa = *xb; *xb = tx; ty = *ya; *ya = *yb; *yb = ty; }
  if (*yb > *yc) { tx = *xb; *xb = *xc; *xc = tx; ty = *yb; *yb = *yc; *yc = ty; }
  if (*ya > *yb) { tx = *xa; *xa = *xb; *xb = tx; ty = *ya; *ya = *yb; *yb = ty; }
}

void trianglefill(byte x1, byte y1, byte x2, byte y2, byte x3, byte y3) {
  /* Sort vertices so (x1,y1) is top, (x2,y2) is middle, (x3,y3) is
   * bottom. After this y1 <= y2 <= y3. */
  sort_by_y(&x1, &y1, &x2, &y2, &x3, &y3);

  /* Degenerate: all three points on one scanline. Draw a single span
   * from the leftmost to the rightmost x and exit — no edges to walk. */
  if (y1 == y3) {
    int lo = x1, hi = x1;
    if (x2 < lo) lo = x2; if (x2 > hi) hi = x2;
    if (x3 < lo) lo = x3; if (x3 > hi) hi = x3;
    span(y1, lo, hi);
    plot_flush();
    return;
  }

  /* Upper half: rows y1..y2. Left/right edges are (v1→v2) and (v1→v3).
   * We don't know a priori which edge is "left" — edge_x returns the
   * absolute x and span() sorts. */
  byte y = y1;
  while (y < y2) {
    int xa = edge_x(y, y1, y2, x1, x2);
    int xb = edge_x(y, y1, y3, x1, x3);
    span(y, xa, xb);
    y++;
  }

  /* Lower half: rows y2..y3. Edges are (v2→v3) and (v1→v3). The y == y2
   * row is handled here so the middle vertex is always included. */
  while (1) {
    int xa = edge_x(y, y2, y3, x2, x3);
    int xb = edge_x(y, y1, y3, x1, x3);
    span(y, xa, xb);
    if (y == y3) break;
    y++;
  }

  plot_flush();
}
