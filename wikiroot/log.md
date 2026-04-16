# Rock Wiki — Log

Append-only chronological record of ingests, queries, lints, and updates.

---

## [2026-04-14] update | RTL component: triangle + filled_triangle

New `src/lib/triangle.{h,c}` exposes two Rock builtins:
`triangle(x1,y1, x2,y2, x3,y3)` (outline — three `draw()` calls) and
`filled_triangle(...)` (scanline-rasterised solid fill including the
outline). Both honour the current draw mode; both inherit y-clipping
from `draw()` per row. "Current ink colour" is implicit — the
framebuffer stores bits and the attribute cell displays the ink set
via [[rtl-ink-paper]], so no special handling is needed in this
component.

**Fill algorithm.** Sort vertices by y, walk upper half with edges
`v1→v2` + `v1→v3`, walk lower half with edges `v2→v3` + `v1→v3`,
emit one `draw(xl, y, xr, y)` per row so each span takes draw.c's
byte-mask H fast path. `plot_flush` once at the end amortises the
host `tb_present`.

**SDCC 16-bit int trap.** The naive edge formula `x0 + (y - y0) *
(x1 - x0) / (y1 - y0)` overflows signed int on Z80 — the intermediate
product can reach 191 × 255 = 48705. Implementation does the
magnitude in `unsigned` (fits 65535) and carries the sign of dx
separately, so it's a single 16-bit multiply + single 16-bit divide
per edge per row.

**Degenerate cases handled:** flat-top, flat-bottom, all-vertices-on-
one-scanline (short-circuited to a single horizontal span), coincident
vertices, bottom-edge overhang beyond y=191.

**Files.** `src/lib/triangle.{h,c}` (new), `src/generator.c` (both
builtins registered + transpile include), `rock` build script
(added to `RTL_C_SRCS`), `test/triangle_test.rkr` (new — outline +
fill + flat-top + flat-bottom + pointy-up + pointy-down + collinear
+ overhang + XOR erase + full-screen),
`wikiroot/pages/rtl/rtl-triangle.md` (new),
`wikiroot/index.md` (entry added).

**Verification.** 243/243 host tests pass. ZXN build produces clean
`triangle_test.nex` (only the expected SDCC info 218 pixelad/setae
warnings from draw.c).

---

## [2026-04-14] update | RTL component: fill (rect; flood deferred)

New `src/lib/fill.{h,c}` ships `fill_rect(byte x0, byte y0, byte x1,
byte y1)` — axis-aligned filled rectangle on the ZX ULA screen. Flood
fill deliberately deferred (needs framebuffer reads + an unbounded
span queue that's awkward to size on the SDCC sdcc_iy stack).

**Implementation.** Twenty lines: normalise endpoints, then walk y
calling `draw(x0, y, x1, y)` per row so each row takes draw.c's
H-fast-path byte-mask walk (one PIXELAD, contiguous `LD (HL), $FF` /
`INC L` loop, leading/trailing partial masks). `plot_flush` once at
the end amortises the host `tb_present` over the whole rectangle.

**Loop shape.** `while (1) { draw; if (y == y1) break; y++; }`
rather than `for (y = y0; y <= y1; y++)` to avoid byte overflow when
`y1 == 255` (academic under the y >= 192 clip, but cheap insurance).

**Mode handling.** Respects the sticky global draw mode for free —
because every row delegates to `draw()`, which already branches on
`rock_draw_mode`. XOR-erase works by calling `fill_rect` twice with
identical coords.

**Files.** `src/lib/fill.{h,c}` (new), `src/generator.c` (builtin
registration + transpile include), `rock` build script (added to
`RTL_C_SRCS`), `test/fill_rect_test.rkr` (new),
`wikiroot/pages/rtl/rtl-fill.md` (new), `wikiroot/index.md` (entry
added).

**Verification.** 242/242 host tests pass. ZXN build produces clean
`fill_rect_test.nex` (only the expected SDCC info 218 pixelad/setae
warnings from draw.c).

---

## [2026-04-14] update | RTL component: circle (outline)

Raster circle outline via the integer midpoint algorithm. New
`src/lib/circle.{h,c}` exposes `circle(byte cx, byte cy, byte r)` as
a Rock builtin. Outline only — fill is a separate component.

**Algorithm.** Classic 8-way symmetric midpoint circle: walk one
octant from `(0, r)` to the 45° diagonal, stamp eight mirrored points
per iteration, branch on the decision variable `d`. `r == 0`
degenerates to a single `plot`.

**Targets.** Single C implementation shared between host and ZXN —
both go through [[rtl-plot]]'s `plot_nopresent` + `plot_flush` API,
so the draw-mode state, per-pixel y-clip, and host batching all come
for free. Deliberately slower than a hand-tuned ZXN routine (which
would cache PIXELAD across the stamped points) in exchange for being
tiny, correct, and identical across targets. Perf work deferred.

**Clipping.** Signed `int` offsets for `cx ± x/y` and `cy ± x/y` so
negative y can be filtered in C before hitting `plot`. Horizontal
offsets rely on byte wrap (256-wide ULA scanline = any byte is valid).

**Files.** `src/lib/circle.{h,c}` (new), `src/generator.c` (builtin
registration + transpile include), `rock` build script (added to
`RTL_C_SRCS`), `test/circle_test.rkr` (new),
`wikiroot/pages/rtl/rtl-circle.md` (new),
`wikiroot/index.md` (entry added).

**Verification.** 241/241 host tests pass. ZXN build produces clean
`circle_test.nex` (only the expected SDCC info 218 pixelad/setae
warnings from draw.c).

---

## [2026-04-14] update | RTL component: polyline

Thin wrapper over [[rtl-draw]]. New `src/lib/polyline.{h,c}` exposes
`polyline(byte[] xs, byte[] ys)` as a Rock builtin — draws N-1
connected segments between paired points by delegating each segment
to `draw()`. Inherits the H/V dispatcher, Bresenham fallback, y-clip,
and current draw mode.

**API shape.** Parallel `byte[]` arrays instead of a `point[]` record
array — Rock's C ABI for user-record arrays isn't shipped yet, and
parallel arrays are zero-ceremony for the caller. Length < 2 is a
no-op; mismatched lengths clamp to the shorter so `byte_get_elem`
never runs out of bounds. Closing a shape is the caller's job
(append the first point again).

**Implementation.** Twenty lines of C using
`__internal_dynamic_array_t` + `byte_get_elem`. Single file, shared
between host and ZXN — there's nothing target-specific because
`draw()` already handles that split. No `polygon` convenience
variant shipped — trivially composable from polyline + one append.

**Files.** `src/lib/polyline.{h,c}` (new), `src/generator.c`
(builtin registration + transpile include), `rock` build script
(added to RTL_C_SRCS), `test/polyline_test.rkr` (new),
`wikiroot/pages/rtl/rtl-polyline.md` (new),
`wikiroot/index.md` (entry added).

**Verification.** 240/240 host tests pass. ZXN build produces clean
`polyline_test.nex` (only the expected SDCC info 218 pixelad/setae
warnings from draw.c).

---

## [2026-04-14] update | Draw modes: OR / XOR for plot + draw

Added a sticky global draw mode shared by [[rtl-plot]] and [[rtl-draw]].
`DRAW_MODE_OR` (0, default) merges additively; `DRAW_MODE_XOR` (1)
toggles the target pixel — draw the same shape twice to erase. Exposed
to Rock as `set_draw_mode(byte)` / `get_draw_mode()`. Two modes instead
of the originally considered three (OR/XOR/ERASE): XOR covers the
erasable-cursor/sprite idiom, halving the asm duplication.

**ZXN implementation.** `plot()` picks between `plot_raw_or` (uses
`or (hl)`) and `plot_raw_xor` (uses `xor (hl)`) — same shape, same
cost. `draw_hline` keeps a single inner loop and branches on mode in
C. `draw_vline` ships two full `__asm` PIXELDN loops (the hot loop
must not test mode). `draw_line_general` puts the if/else inside the
Bresenham loop body, around the per-pixel inline `__asm` — ~50% slower
under XOR than under OR but half the code size; duplicating the full
shallow/steep Bresenham was rejected. Host path adds `shadow_xor` +
a mode branch in `plot_nopresent`.

**Files.** `src/lib/plot.{h,c}` (state + API + `rock_draw_mode`),
`src/lib/draw.c` (hline/vline/Bresenham mode branches),
`src/generator.c` (`set_draw_mode`/`get_draw_mode` builtin registration),
`test/draw_mode_test.rkr` (new), `wikiroot/pages/rtl/rtl-plot.md`,
`wikiroot/pages/rtl/rtl-draw.md` (both updated).

**Verification.** 239/239 host tests pass. ZXN builds of
`draw_mode_test`, `plot_test`, `draw_test` all produce clean `.nex`
files (only the expected SDCC info 218 pixelad/setae mnemonic-size
warnings).

---

## [2026-04-14] update | RTL component: draw (raster line, Phase 1)

Second raster-graphics RTL component, sibling to [[rtl-plot]]. New
`src/lib/draw.{h,c}` exposes `draw(byte x0, byte y0, byte x1, byte y1)`
as a Rock builtin — draws an additive straight line between two absolute
pixel coordinates on the ZX ULA screen. Origin top-left, pixels with
y>=192 skipped per-iteration.

**Dispatcher** branches on line shape. Horizontal (`y0 == y1`) takes a
byte-mask walk: one `PIXELAD`, then `INC L` across the scanline with
leading/trailing partial-byte masks and `LD (HL),0xff` for full bytes
between. Vertical (`x0 == x1`) takes a `PIXELDN` loop with the pixel
mask cached in B (one `SETAE` at the top, never recomputed). General
case is a C Bresenham, shallow/steep split, with the per-pixel write
inlined as a literal `__asm` block — no function call, no stack
ceremony; six Z80 ops per iteration (2 scratch loads + PIXELAD + SETAE
+ OR (HL) + store). Single-pixel degenerate (`x0==x1 && y0==y1`) routes
straight to `plot()`.

**Hand-rolled asm Bresenham deferred** to Phase 2 — the C-driven
inline-asm version is correct on first ship, debuggable, and
fast enough that the H/V fast paths dominate real workloads. 45°
diagonal fast path skipped because Bresenham at dx==dy is already
optimal. Off-screen whole-line clipping also Phase 2.

**Host path** is a single C Bresenham loop (no dispatcher — termbox
redraw dominates, asm tricks wouldn't help). Uses new `plot_nopresent`
+ `plot_flush` helpers exposed from `plot.h` so `draw.c` doesn't need
to know about the shadow buffer or `tb_present`: one flush per line,
N shadow writes + cell redraws between. `plot()` itself now decomposes
into `plot_nopresent + plot_flush`, transparent to Rock programs.

**State sharing** between `plot.c` and `draw.c` goes only through
`plot.h`'s public API. `draw.c` has its own scratch bytes
(`rock_draw_x`, `rock_draw_y`, `rock_draw_count`, `rock_draw_hl`) for
its own inline asm — no reach into `plot.c`'s internals.

Verified: **235/235 host tests pass** (234 prior + 1 new `draw_test.rkr`
covering every dispatcher branch, shallow, steep, reversed endpoints,
y-clip, and a box outline). `./rock --target=zxn test/draw_test.rkr`
produces a clean `draw_test.nex`. SDCC emits `info 218` messages for
`pixelad` / `setae` because its code-size estimator doesn't know Z80N
mnemonics, but the assembler handles them and the binary is correct.
`plot_test.nex` also still builds clean (same Z80N deps).

Pages created: rtl/rtl-draw.md
Pages updated: index.md, rtl/rtl-plot.md (cross-ref left as-is — plot's
shadow/flush split is now documented in rtl-draw)

---

## [2026-04-14] update | RTL component: plot (raster pixel, Phase 1)

First raster-graphics RTL component. `plot(byte x, byte y)` sets a single
pixel on the ZX ULA screen. ZXN path writes the framebuffer at `$4000-$57FF`
directly using Z80N `PIXELAD` + `SETAE` — four real instructions, no ROM
dependency. `src/lib/plot.c` uses the same file-scope-scratch-byte pattern
as `print_at.c`/`ink_paper.c` so the naked inline-asm routine reads args
with absolute `LD A,(nn)` and sidesteps the SDCC calling convention.

Host path keeps a 256×192 bit shadow matching the real ULA bitmap (1 bit
per pixel, MSB-left) and renders it through termbox2 using 16 Unicode
quadrant block glyphs (`▘▝▀▖▌▞▛▗▚▐▜▄▙▟█`). Each terminal cell covers a 2×2
pixel region → 128×96 visible resolution — coarse but enough to eyeball
shapes. `host_caps.plot` flag gates the termbox branch; piped-stdout test
harness falls back to `plot(x,y)\n` log lines. Top-left origin (matches
`print_at`/termbox2, not Sinclair BASIC), additive-only (OR), no
XOR/unplot/toggle variants in Phase 1. y≥192 clipped silently.

Plot does *not* depend on [[rtl-ink-paper]] attribute state on either
target in Phase 1 — the shadow is monochrome-in-TB_WHITE on host, and
on ZXN the attribute cell the pixel lands in is whatever CLS left there.
A later extension can read `attr_fg`/`attr_bg` during `redraw_cell`.

Verified: 234/234 host tests pass; `./rock --target=zxn test/plot_test.rkr`
produces a clean `plot_test.nex`. `host_caps.plot` added to
[[rtl-host-caps]]; `plot.h` included in `transpile()`; `plot.c` appended
to `RTL_C_SRCS`. See [[rtl-plot]] for the full component design.

Pages created: rtl/rtl-plot.md
Pages updated: index.md

---

## [2026-04-13] update | print_at host path via termbox2

Replaced the plain-stdout host branch of `print_at` with a termbox2-backed renderer so gcc builds of Rock programs using `print_at` can be previewed in a real terminal at something close to the ZX 32×24 layout. termbox2 is vendored at `src/ext/lib/termbox2.h` and now has a dedicated one-TU impl file at `src/lib/host/termbox2_impl.c` that defines `TB_IMPL` once; other host components can `#include "termbox2.h"` without worrying about multiple-definition errors.

`print_at.c` host branch is a 3-state lazy-init machine: on first call it checks `isatty(STDOUT_FILENO)`. Not a tty (e.g. test harness piping) → plain-text fallback, matching the old behaviour. Is a tty → `tb_init` + `atexit(tb_shutdown)` and subsequent calls go through `tb_print` + `tb_present`. This keeps `./run_tests.sh` working unchanged.

Build script gained a new `RTL_HOST_SRCS` variable (host-only extra TUs) and the gcc command line adds `-I src/ext/lib`. SDCC branch is untouched and termbox2 never enters the ZXN build because of the existing `#ifdef __SDCC` split. A local `#pragma GCC diagnostic ignored "-Wunused-function"` in `termbox2_impl.c` suppresses a benign termbox2-internal warning under -Wall.

Host tests: 225/225. ZXN build: `print_at_test.nex` still clean.

Convention addition: host-only third-party deps now go under `src/lib/host/` with a one-TU impl file referenced from `RTL_HOST_SRCS`. [[rtl-overview]] updated with the recipe.

Pages updated: rtl/rtl-print-at.md, rtl/rtl-overview.md

---

## [2026-04-13] update | RTL component: print_at (ROM placeholder)

Third RTL component. `print_at(byte x, byte y, string text)` draws text at a character cell on the ZX upper screen. ZXN path calls ROM `RST 10h` directly (inline asm in `src/lib/print_at.c`), emitting the AT control sequence `22, row, col` followed by each text byte. Host path writes `@(x,y) text\n` to stdout for inspection. Implementation uses a file-scope scratch byte + noarg inline-asm wrapper to sidestep any SDCC calling-convention concerns. Deliberately bypasses z88dk's stdio control-code layer to stay close to the metal and reduce footprint.

Host tests: 225/225. ZXN build: `print_at_test.nex` clean on first try, ~33 KB.

Pages created: rtl/rtl-print-at.md
Pages updated: index.md

## [2026-04-13] todo | print_at raster replacement

The current `print_at` ZXN implementation depends on ROM being paged in at `$0000-$3FFF` and uses the stock ZX character set via ROM. Future Rock programs will need to swap the ROM out for RAM banks and/or use custom fonts. Build a raster version that writes glyph bytes directly into the ULA framebuffer at `$4000-$57FF`, honouring the eccentric Y-bit permutation (`addr = 0x4000 | ((r & 0x18) << 5) | ((r & 0x07) << 5) | (c & 0x1F)`, scanlines step by `+0x100`). Likely location: `src/lib/zxn/raster.asm`. On landing, switch `print_at` to the raster path and retire or demote the ROM path. See [[rtl-print-at]].

## [2026-04-13] todo | Rock builtin / function overloading

`print_at(x, y, text)` was chosen over overloading `print(text)` because Rock's current call dispatch is name-only — the name table has no arity or type-based overload resolution. Adding overload support is a compiler feature of its own (parser changes, name-table keyed on signature, generate_funcall dispatch). When it lands, `print_at` can be re-exposed as `print(x, y, text)` without touching the C implementation. Until then, flat distinct names are the convention. See [[rtl-print-at]] and [[rtl-overview]].

---

## [2026-04-12] update | RTL component: border (Phase C — first follow-on)

Second RTL component to validate that the Phase A conventions generalise. `border(colour)` and `border_get()` added as builtins; implementation in `src/lib/border.h`/`src/lib/border.c` using z88dk's `<z80.h>` `z80_outp` on ZXN and a shadow byte on host. Key result: **no `.asm` file was needed**, which exercises the C-only degenerate case of the component convention. Compiler/build diff is ~4 lines. Host tests: 224/224 (221 + 3 new). ZXN build: `border_test.nex` clean on first try — no SDCC symbol gotchas because nothing new is exported from asm.

Lesson captured: prefer C + `<z80.h>` for simple port I/O; reserve `.asm` files for genuinely performance-sensitive or register-dependent code.

Pages created: rtl/rtl-border.md
Pages updated: index.md

---

## [2026-04-12] update | RTL pilot: keyboard scanner (Phase A)

First RTL component delivered under the strategy in [[pasta80-lessons-for-rock]]. Added `src/lib/keyboard.h`, `src/lib/keyboard.c`, `src/lib/zxn/keyboard.asm`, and `test/keyboard_test.rkr`. Generator now registers `scan_keyboard`/`key_pressed` as builtins and emits `#include "keyboard.h"`. Build script adds `keyboard.c` to `LIB_SRCS` and `keyboard.asm` to the zcc invocation. Host test suite: 221/221 pass (217 baseline + 4 new). ZXN build verified: `keyboard_test.nex` (~33 KB) produced cleanly.

Key lesson captured: **SDCC prefixes every C extern with a leading underscore when emitting asm**. Both functions and data symbols must declare `PUBLIC _name` and (for data) `DEFC _name = label`. The initial ZXN build failed with `undefined symbol: _ZK_BUFFER` until the buffer alias was renamed. Now documented in [[rtl-overview]] as the canonical gotcha for future components.

Pages created: rtl/rtl-overview.md, rtl/rtl-keyboard.md
Pages updated: index.md, pasta80/pasta80-lessons-for-rock.md (marked partially applied)
TODOs filed: none

---

## [2026-04-12] ingest | pasta80

Ingested the PASTA/80 Pascal cross-compiler project as a reference system for Rock's cross-platform RTL strategy. PASTA/80 targets Z80 platforms (CP/M, ZX Spectrum 48K/128K, ZX Spectrum Next, Agon Light) and has a mature 4-layer RTL architecture with a clean HAL pattern (Block* procedures) that allows shared file I/O code across all platforms with file system access. Key lessons identified: adopt the Block* HAL pattern with byte-oriented I/O, use layered file composition, keep mixed C/ASM split. Key improvements over PASTA/80: use return codes instead of global error state, add heap coalescing, ensure dead-code elimination by default.

Pages created: pasta80-overview.md, pasta80-rtl-architecture.md, pasta80-rtl-api.md, pasta80-target-platforms.md, pasta80-lessons-for-rock.md
Pages updated: index.md
TODOs filed: none

---

## [2026-04-09] ingest | Bootstrap from src/ and test/

**Sources:** Direct bootstrap from `../src/` and `../test/` (not via new/ lifecycle — initial wiki creation).

**Files read:** `src/lexer.c`, `src/token.c`, `src/token.h`, `src/parser.c`, `src/ast.c`, `src/ast.h`, `src/generator.c`, `src/generator.h`, `src/name_table.c`, `src/main.c`, `src/lib/fundefs_internal.c`, `src/stringview.h`, `src/alloc.h`, `docs/SYNTAX.md`, all `test/*.rkr` files.

**Pages created (20):**

- `README.md` — wiki orientation and workflow guide
- `index.md` — content catalog
- `pages/overview.md` — high-level architecture synthesis
- `pages/ubiquitous-language.md` — domain glossary (28 terms)
- `pages/domain-model.md` — bounded context map with Mermaid diagram
- `pages/lexer/lexer-overview.md` — lexer internals
- `pages/parser/parser-overview.md` — parser internals and grammar
- `pages/generator/generator-overview.md` — code generation internals
- `pages/targets/host-gcc.md` — host target
- `pages/targets/zxn-z80.md` — ZXN/Z80 target
- `pages/testing/testing-overview.md` — test suite (27 tests)
- `pages/syntax/syntax-index.md` — keyword + builtin reference
- `pages/syntax/types.md` — type system and variable declarations
- `pages/syntax/control-flow.md` — if, while, for, match, operators
- `pages/syntax/functions-and-methods.md` — sub, methods, mangling
- `pages/syntax/arrays.md` — array operations
- `pages/syntax/strings.md` — string operations
- `pages/syntax/modules-and-records.md` — record, pro, enum, module
- `pages/syntax/embed.md` — @embed blocks
- `pages/concepts/compilation-pipeline.md` — pipeline step-by-step
- `pages/concepts/name-table.md` — symbol table design
- `pages/concepts/string-representation.md` — rock_string and temporaries
- `pages/concepts/array-internals.md` — __internal_dynamic_array_t

**Pages updated:**

- `wiki-proposal.md` — fixed incorrect `syntax/` category description (copy-paste artefact from unrelated project)

**Key architectural takeaways:**
- Three-phase pipeline: lex → parse → generate (transpile to C, not compile to machine code)
- Two targets: host (gcc, absolute includes) and ZXN (Z88DK/SDCC, relative includes, statement splitting)
- No semantic analysis phase — type inference and error detection happen inside the generator
- Arena allocator spans all phases; no incremental freeing
- Type-specific array wrappers synthesised by generator at each use site
- pre_f buffer pattern handles C statement/expression ordering constraints

---

## [2026-04-09] update | Fix rock/rockc/run_tests.sh documentation
Pages updated: `testing-overview.md`, `host-gcc.md`, `overview.md`.
Corrections: removed false `--target=zxn` flag from run_tests.sh docs (script is gcc-only); added single-file test mode. Clarified `rock` (full build script) vs `rockc` (transpiler binary) distinction. Fixed default output name (`<basename>.exe` not `out`). Added `--debug` flag documentation.

---

## [2026-04-10] lint | Link validity check
Issues found: 1 content, 0 structural, 2 TODOs.
- **Fixed:** `index.md` line 3 — `[[filename]]` format-example was a false wiki-link; replaced with plain text.
- **Links:** 35 unique `[[links]]` across all pages; 34/34 resolve correctly. 0 orphans. Index perfectly matches pages (34/34).
- **Images:** 46 image references across ZXN pages; all 46 resolve correctly to `raw/images/`.
- **TODOs outstanding:** (1) nested sub unimplemented in `functions-and-methods.md`; (2) `pro` type match semantics in `modules-and-records.md`.
- **Pending sources:** none in `new/`.

---

## [2026-04-10] ingest | zxnext_guide.md — ZX Spectrum Next hardware reference (Chapter 3)

**Source:** `raw/zxnext_guide.md` (10,177 lines; Tomaz Kragelj, 2022). Chapter 3 only — Chapter 2 (Z80 CPU) deferred.
**Images:** 195 images moved to `raw/images/` (referenced from pages as `../../../raw/images/`).

**Pages created (13):**

- `pages/targets/zxn-hardware.md` — ZXN hardware overview: layer stack, port access patterns, implementer notes
- `pages/targets/zxn/zxn-ports-registers.md` — All I/O ports + full Next register index with subsystem cross-links
- `pages/targets/zxn/zxn-memory-paging.md` — 8K/16K banking, MMU slots `$50`–`$57`, legacy 128K/+3 modes
- `pages/targets/zxn/zxn-dma.md` — zxnDMA: WR0–WR6, prescalar, examples, interrupt interaction
- `pages/targets/zxn/zxn-palette.md` — 8-bit/9-bit colour, palette editing, Layer 2 priority flag
- `pages/targets/zxn/zxn-ula.md` — Pixel/attribute memory, border, shadow screen, PIXELAD/PIXELDN/SETAE
- `pages/targets/zxn/zxn-layer2.md` — 256×192/320×256/640×256 modes, bank paging, double buffering
- `pages/targets/zxn/zxn-tilemap.md` — Tile definitions, 2-byte/1-byte tilemap data, stencil mode
- `pages/targets/zxn/zxn-sprites.md` — Pattern upload, 8/4-bit colour, anchor/composite/unified groups
- `pages/targets/zxn/zxn-copper.md` — WAIT/MOVE/HALT/NOOP, 2KB program memory, upload via `$63`
- `pages/targets/zxn/zxn-sound.md` — AY-3-8912 ×3 (Turbo Sound), registers 0–13, chip select
- `pages/targets/zxn/zxn-keyboard.md` — 8×5 matrix via `$xxFE`, extended keys `$B0`/`$B1`
- `pages/targets/zxn/zxn-interrupts.md` — IM1/IM2/Hardware IM2, 14-source vector table, line interrupt

**Pages updated:**
- `pages/targets/zxn-z80.md` — added ZXN Hardware Reference section linking all subsystem pages
- `pages/ubiquitous-language.md` — added 13 new terms: AY-3-8912, Bank, Copper, DMA, Layer 2, MMU Slot, Next Register, NEXTREG, Palette, Palette Offset, Sprite, Tilemap, ULA
- `pages/domain-model.md` — added ZXN Hardware bounded context + Mermaid nodes; added ZXN Hardware Context description
- `pages/overview.md` — added ZXN hardware reference mention in ZXN target section
- `index.md` — added hub page + 12 subsystem pages under new "ZXN Hardware Subsystems" section

**Key takeaways:**
- Next registers accessed via `NEXTREG` (20–24 T-states) — preferred over `$243B`/`$253B` ports (52–58 T-states)
- Four compositable graphics layers: ULA (base), Layer 2 (framebuffer), Tilemap (tiles), Sprites (top). Priority via `$15`.
- Layer 2 requires MMU bank swapping (8K at a time via slot 6) — too large for Z80 address space
- Sprite patterns stored in FPGA RAM — DMA via port `$xx5B` is the efficient load mechanism
- Copper co-processor eliminates per-scanline ISR overhead for raster effects
- DMA with prescalar enables sampled audio streaming at fixed sample rates
- Hardware IM2 provides 14 independently vectored interrupt sources on a 32-byte-aligned table

---

## [2026-04-09] todo | pro type match semantics unclear

**Issue:** The exact match semantics for `pro` (product type) values — specifically how match arms access the constructor payload — need clarification from source and tests. The `modules-and-records.md` page notes this with a TODO block.

**Pages affected:** `pages/syntax/modules-and-records.md`

**Resolution:** Ingest `test/` files that use `pro` types, or ask the architect to clarify.

## [2026-04-10] lint | fix zxn target docs and nested-sub reference

Changed files: 3

Issues fixed: corrected the ZXN target page to reflect the current rock/rockc/zcc workflow, include strategy, and runtime source paths; corrected the nested-sub TODO to point at generator.c.
Remaining issues: frontmatter/source attribution drift remains on overview.md and domain-model.md; pro match semantics TODO remains unresolved.
Link check: validated all [[...]] targets under wikiroot/pages; 34/34 page targets resolve to matching .md files.
TODOs filed: none


## [2026-04-10] lint | fix parser overview include resolution notes

Changed files: 2

Issues fixed: corrected parser_t field descriptions, updated include resolution to say paths are resolved relative to the including file, and corrected the nested-subs note to reference generator.c.
Remaining issues: none in the touched page from this pass.
TODOs filed: none


## [2026-04-10] lint | refresh syntax docs after declaration and array-field changes

Changed files: 15

Issues fixed: removed stale let/dim syntax from language pages, updated parser docs for type-first declarations and generic postfix chains, documented record-field arrays and expression receivers, corrected module examples and ZXN header-resolution notes, and refreshed testing docs to match the current harness and top-level suite.
Remaining issues: one intentional parser note still mentions removed let/dim syntax only to state that they are no longer accepted; broader wiki lint coverage outside syntax/build topics remains for a later pass.
TODOs filed: none


## [2026-04-10] lint | remove stale include-path and pipeline misconceptions

Changed files: 6

Issues fixed: removed stale claims that host builds emit absolute runtime include paths; updated pipeline examples to current new_parser(prog) and new_generator(cout) usage; aligned host/ZXN target docs with the driver-managed -I "$ROCK_ROOT/src/lib" include strategy.
Remaining issues: none found for the targeted misconception scan.
TODOs filed: none.


## [2026-04-10] lint | neutralise wiki review-tone and fix host target toolchain docs

Changed files: 10

Issues fixed: corrected the host target toolchain table so rock and rockc roles match the code; removed defensive wording about copied headers and old absolute-include behaviour; removed review-oriented phrasing such as "pre-existing, not a regression"; simplified syntax and overview pages to state current behaviour directly.
Remaining issues: none found in the targeted neutral-tone sweep across the touched pages.
TODOs filed: none.

## [2026-04-11] ingest | ZXN interrupt samples

Changed files: 16

Summary: Ingested the IM1, legacy IM2, Hardware IM2, and bus-safe IM2 assembly samples. Added a ZXN sample-program hub, an interrupt topic overview, and per-source summaries that explain setup order, memory placement, vector-table shape, and runtime implications.
Pages created: zxn-sample-programs, zxn-interrupt-samples, zxn-im1-sample-summary, zxn-im2-sample-summary, zxn-im2hw-sample-summary, zxn-im2safe-sample-summary.
Pages updated: zxn-interrupts, domain-model, overview, ubiquitous-language, index.
Review/lint: verified touched wiki links and raw source links; fixed a pre-existing glossary link typo while updating glossary terms.
TODOs filed: none.

## [2026-04-11] todo | Confirm sprite sample DMA length preservation

**Issue:** `samples/sprites/main.asm` documents `BC` as the sprite byte count for `loadSprites`, but the routine loads `$303B` into `BC` before storing `.dmaLength`. The sample therefore appears to upload `$303B` bytes instead of the requested `16*16*5` bytes.

**Pages affected:** `pages/targets/zxn/samples/zxn-sprite-sample-summary.md`, `pages/targets/zxn/zxn-dma.md`

**Resolution:** Confirm whether the sample intentionally uploads more of `sprites.spr`, or update the sample/routine pattern to preserve the caller's `BC` byte count before selecting the sprite slot port.

## [2026-04-11] ingest | Z88DK tools and assembly reference

**Sources (11):** `z88dk_inlineAssembler.md`, `z88dk-calling-with-stack-params.md`, `z88dk_z80.md`, `Tool---z80asm.md`, `Tool---z80asm---command-line.md`, `Tool---z80asm---input-format.md`, `Tool---z80asm---preprocessor.md`, `Tool---z80asm---expressions.md`, `Tool---z80asm---directives.md`, `Tool---z80asm---environment.md`, `Tool---z80asm---old-manual.md` (stub only).

**Pages created (3):**
- `pages/targets/zxn/z88dk-inline-asm.md` — `#asm`/`#endasm` syntax, Z88DK inline assembly guidance, calling conventions, data type sizes, stack layout, return registers, safe `@embed asm` patterns for SDCC/IY
- `pages/targets/zxn/z88dk-z80-library.md` — `<z80.h>` library: `z80_delay_ms`, port I/O (`z80_inp`/`z80_outp`/block variants), memory access macros, interrupt state, full IM2 management API
- `pages/targets/zxn/z80asm-reference.md` — z80asm assembler/linker: three modes, key CLI flags, source format (labels, local labels, number literals, continuation), expression operator table, preprocessor (`#define`, `MACRO`/`ENDM`, `REPT`/`REPTC`/`REPTI`, `DEFL`), all key directives (`DEFB`/`DEFW`/`DEFS`/`DEFVARS`/`DEFGROUP`, `ORG`/`SECTION`/`ALIGN`/`PHASE`, `PUBLIC`/`EXTERN`, `IF`/`IFDEF`, `CU.*`, `DMA.*`), file types, environment variables

**Pages updated (4):**
- `pages/targets/zxn-z80.md` — added "Z88DK Tools and Assembly" hub section with links to all three new pages
- `pages/ubiquitous-language.md` — added `z80asm`, `ASMPC`, `Calling Convention`
- `index.md` — added "Z88DK Tools and Assembly" subsection with 3 entries
- `wikiroot/log.md` — this entry

**Key takeaways from sources:**
- Z88DK recommends **against** heavy inline assembly; prefer separate `.asm` files with C prototypes for callable assembly functions
- `#asm`/`#endasm` is processed by `zcc` before SDCC sees the file — this is how Rock's `@embed asm` generator output works
- SDCC (`-clib=sdcc_iy`) uses IY as frame pointer — never modify IY inside `@embed asm`
- Stack layout at call entry (sccz80 / stack-mode): SP = return addr, SP+2 = 2nd param, SP+4 = 1st param; return values in L (byte), HL (int/word), DE:HL (long)
- z80asm supports ZXN-native `CU.*` (Copper) and `DMA.*` directives for producing encoded hardware control bytes in assembly source
- `ASMPC` in z80asm = current instruction address, resolved at link time
- z80asm `-mz80n` enables ZX Spectrum Next Z80N extended instruction set

---

## [2026-04-11] update | Fix @embed asm documentation accuracy

Changed files: 2 (`pages/syntax/embed.md`, `pages/targets/zxn-z80.md`)

Issues fixed:
- **embed.md**: "Host builds reject it" was wrong — host builds silently skip the block via `#ifdef __SDCC`; gcc compiles the surrounding function normally. Rewrote the Generator Handling section to accurately describe that `@embed c` emits verbatim while `@embed asm` wraps in `#ifdef __SDCC` / `#asm` / `#endasm` / `#endif`. Added the new `test/embed_asm_test.rkr` to test coverage.
- **zxn-z80.md**: Known Limitations row claimed assembly "requires `__asm`/`__endasm` syntax" — wrong. The generator uses Z88DK's `#asm`/`#endasm` preprocessor directives (handled by `zcc` before SDCC). Fixed. Also corrected `asm_interop` path from `src/lib/z80/` to `src/lib/`.

---

## [2026-04-11] ingest | ZXN sprite sample

Changed files: 10

Summary: Ingested the sprite assembly sample and binary sprite sheet. Added a sprite sample summary covering DMA pattern upload, sprite attribute writes, and unified relative sprite groups.
Pages created: zxn-sprite-sample-summary.
Pages updated: zxn-sample-programs, zxn-sprites, zxn-dma, ubiquitous-language, index.
Review/lint: verified touched wiki links and raw asset links; checked sprite asset size/hash; drained the sprite pending directory including its build .gitignore.
TODOs filed: 1 — confirm whether loadSprites intentionally clobbers BC before writing the DMA length.


## [2026-04-11] ingest | ZXN sound sample

Changed files: 8

Summary: Ingested the sound assembly sample. Added a sound sample summary covering Turbo Sound setup, AY register selection/writes, the compact tune-row format, and the CSpect/OpenAL runtime caveat.
Pages created: zxn-sound-sample-summary.
Pages updated: zxn-sample-programs, zxn-sound, ubiquitous-language, index.
Review/lint: verified touched wiki links and raw source links; drained the sound pending directory including its build .gitignore.
TODOs filed: none.


## [2026-04-11] ingest | ZXN tilemap sample

Changed files: 11

Summary: Ingested the tilemap assembly sample and its map, tile, and palette binary assets. Added a tilemap sample summary covering bank 5 placement, one-byte 40x32 entries, tile definitions, palette upload, and offset-based shake.
Pages created: zxn-tilemap-sample-summary.
Pages updated: zxn-sample-programs, zxn-tilemap, ubiquitous-language, index.
Review/lint: verified touched wiki links and raw asset links; checked asset sizes/hashes; drained the tilemap pending directory including its build .gitignore.
TODOs filed: none.


## [2026-04-11] ingest | ZXN Layer 2 samples

Changed files: 15

Summary: Ingested the three Layer 2 graphics samples for 256x192, 320x256, and 640x256 modes. Added a comparison page and per-mode summaries, and corrected the Layer 2 reference to show practical 8K slot-6 addressing formulas.
Pages created: zxn-layer2-samples, zxn-layer2-256x192-sample-summary, zxn-layer2-320x256-sample-summary, zxn-layer2-640x256-sample-summary.
Pages updated: zxn-sample-programs, zxn-layer2, ubiquitous-language, index.
Review/lint: verified touched wiki links and raw source links; checked source moves and patch-marker cleanup. Non-source .DS_Store metadata remains pending for final cleanup/reporting.
TODOs filed: none.

## [2026-04-11] todo | Confirm Copper sample Layer 2 clear bank count

**Issue:** `samples/copper/main.asm` defines `L2_END_8K_BANK = L2_START_8K_BANK + 6` and exits after comparing with `L2_END_8K_BANK + 1`, which appears to clear seven 8K banks (`18..24`) instead of the six 8K banks required for 256x192 Layer 2.

**Pages affected:** `pages/targets/zxn/samples/zxn-copper-sample-summary.md`

**Resolution:** Confirm whether the extra bank clear is intentional padding or an off-by-one in the sample constants.

## [2026-04-11] ingest | ZXN copper sample

Changed files: 9

Summary: Ingested the Copper assembly sample. Added a Copper sample summary covering Layer 2 setup, Copper list upload via $63 or DMA, live one-byte patching via $60, palette changes, and clip-window effects.
Pages created: zxn-copper-sample-summary.
Pages updated: zxn-sample-programs, zxn-copper, zxn-dma, ubiquitous-language, index.
Review/lint: verified touched wiki links and raw source links; drained the copper pending directory including its build .gitignore.
TODOs filed: 1 — confirm whether the Layer 2 clear loop intentionally clears seven 8K banks.


## [2026-04-11] ingest | ZXN sample support scaffolds

Changed files: 6

Summary: Moved remaining sample support .gitignore scaffold files from new/ to raw/. These files preserve build/tool directory placeholders that accompany the ingested assembly samples.
Pages created: none.
Pages updated: none.
Review/lint: confirmed remaining pending files are only .DS_Store metadata.
TODOs filed: none.

## [2026-04-11] todo | Nested sub definitions unimplemented

**Issue:** `pages/syntax/functions-and-methods.md` notes that nested `sub` definitions are still unimplemented; `generator.c` currently asserts `sub.path.length == 1`.

**Pages affected:** `pages/syntax/functions-and-methods.md`

**Resolution:** Decide whether nested functions should remain unsupported syntax, become a parser-level error, or gain generator/runtime semantics.

## [2026-04-11] lint | fix wiki lint follow-ups

Changed files: 4

Issues fixed: removed pending .DS_Store metadata files from new/, added a formal TODO log entry for the nested-sub inline TODO, sorted ubiquitous-language glossary headings alphabetically, and committed the source-checked embed/Z80 target documentation updates already present in the wiki working tree.
Remaining issues: content TODOs remain open for pro match semantics, nested sub semantics, sprite sample DMA length preservation, and Copper sample Layer 2 bank count.
Validation: verified new/ has no pending files, glossary ordering has no out-of-order pairs, and wiki links/frontmatter/index coverage remain clean.
TODOs filed: 1 — nested sub definitions unimplemented.


## [2026-04-11] ingest | syntax coverage and Z88DK startup CRT

Changed files: 15

Summary: Ingested the Z88DK startup CRT note and updated the ZXN target pages with source-backed startup profile details. Audited docs/SYNTAX.md against lexer.c, parser.c, generator.c, and src/lib runtime headers, then expanded the syntax wiki so it has equivalent or better coverage without carrying stale claims.
Pages created: pages/targets/zxn/zx-z88dk-startupcrt-summary.md, pages/syntax/builtins-and-io.md, pages/syntax/comments-and-includes.md
Pages updated: pages/targets/zxn-z80.md, pages/ubiquitous-language.md, pages/syntax/types.md, pages/syntax/strings.md, pages/syntax/modules-and-records.md, pages/syntax/functions-and-methods.md, pages/syntax/control-flow.md, pages/syntax/arrays.md, pages/syntax/syntax-index.md, index.md
Review/lint: verified 51 pages, 435 wiki links, 51 index links, sorted glossary, and no pending sources. Excluded stale SYNTAX.md claims about dim/let declarations, bool, slash-only comments, loop keyword use, and substring length semantics because source does not support them as written.
TODOs filed: none.

## [2026-04-11] ingest | Z88DK tools and inline assembly documentation

Changed files: 8

Summary: Ingested 11 Z88DK documentation sources from wikiroot/new/ covering inline assembly calling conventions, the z80.h C library, and the z80asm assembler/linker. Organised into three new pages under targets/zxn/. Updated zxn-z80.md and index.md with hub links. Added z80asm, ASMPC, and Calling Convention terms to ubiquitous-language.md.
Pages created: pages/targets/zxn/z88dk-inline-asm.md, pages/targets/zxn/z88dk-z80-library.md, pages/targets/zxn/z80asm-reference.md
Pages updated: pages/targets/zxn/zxn-z80.md, pages/ubiquitous-language.md, index.md
TODOs filed: none.

## [2026-04-11] update | @embed asm documentation accuracy

Changed files: 2

Summary: Corrected five inaccuracies in syntax/embed.md and targets/zxn-z80.md found by verifying against src/generator.c. Fixed: opening paragraph claim that asm body is "emitted unchanged" (it is wrapped); removed "Host builds reject @embed asm" claim (silently skipped via #ifdef __SDCC); corrected #asm/#endasm directive names (was incorrectly __asm__/__endasm__); fixed asm_interop library path; added Generator Handling table, Host Behaviour section, embed_asm_test.rkr to test coverage list, and link to z88dk-inline-asm.
TODOs filed: none.

## [2026-04-11] lint | full wiki health check

Changed files: 6

Issues fixed:
- testing-overview: test count 38→39, added 9 missing test rows, updated status date
- syntax/embed.md: added missing cross-reference to z88dk-inline-asm in See Also
- zxn-copper.md: added [[z80asm-reference]] link for CU.* directives
- zxn-dma.md: added [[z80asm-reference]] link for DMA.* directives

Validation results:
- Links: 54/54 valid (0 broken)
- Orphans: 0
- Index coverage: 54/54 pages indexed
- Frontmatter: 54/54 pages valid

Structural note: targets/zxn/ has 17 pages (guideline ≤15 after adding 3 new Z88DK pages). Candidate reorganisation: move z88dk-inline-asm, z88dk-z80-library, z80asm-reference to targets/zxn/tools/ — held pending user approval due to git history implications.
TODOs filed: none.

## [2026-04-11] lint | first automated lint with wikiroot/tools/lint.py

Automated tool: 1 issue (pending sources only). Manual checks: 0 content, 0 structural.
Tool results: 54/54 frontmatter valid, 471/471 links valid, 0 orphans, 54/54 indexed, all dirs within threshold.
Pending: wikiroot/new/samples/ awaiting ingest (sjasmplus samples — will resolve TODOs 1 & 2).
Outstanding TODOs: 4 open (copper bank count, sprite DMA length, nested subs, pro match semantics).


## [2026-04-13] update | RTL host capability layer

Refactored host lifecycle out of `print_at.c` into a new shared module `src/lib/host_caps.{h,c}`. `rock_rtl_init()` runs once at program startup (emitted by `transpile_fundef` between `fill_cmd_args` and the user body), `rock_rtl_shutdown()` runs once before `return 0`. ZXN init is trivial (set every flag to 1); host init does the `isatty` probe, `tb_init`, and installs the `atexit` teardown for termbox2. `print_at.c`'s host branch is now a flat `host_caps.print_at` flag check — no per-call lazy init, no state machine. Added rule 11 to `rtl-overview.md` ("lifecycle lives in host_caps, not in components") and a new page `rtl-host-caps.md`. Verified: 225/225 host tests pass, `print_at_test.nex` builds cleanly on ZXN.

## [2026-04-13] update | Function overloading (arity-only, Phase 1)

Shipped arity-based overloading for top-level user functions. Two+ `sub name(...)` declarations may now share a name as long as their arities differ; the generator mangles the C symbol to `name__N` only when a name has ≥2 fundefs (single-definition names stay bare — no diff churn). Implementation: new `program` field on `generator_t`, helpers `program_user_fundef_count` + `emit_fun_name` in `src/generator.c`, wired at the three emission sites (forward decl, definition, `generate_funcall`). No changes to the lexer, parser, name table, or typechecker. Builtins are not overloaded in Phase 1 (user function vs builtin still shadows). Hardcoded builtin fast-paths (append/get/set/pop/insert/substring/concat/to_string/printf) unchanged. Verified: 229/229 host tests pass (225 prior + 4 new `overload_test.rkr` assertions), ZXN build of `overload_test.nex` clean. See `pages/decisions/ADR-0001-function-overloading-arity-only.md` for the rationale. Phase 2 (RTL consolidation: `print_at` → `print(x,y,text)` overload) is a separate follow-up.

## [2026-04-13] update | RTL consolidation: print(x,y,text) — Phase 2 of overloading

Retired the Rock-level name `print_at` and re-exposed the same C implementation as the 3-arg overload of `print`. Rock source now uses `print("hi")` for the 1-arg form and `print(x, y, "hi")` for the positioned form. Mechanism: a dedicated fast-path branch in `generate_funcall()` that recognises `print/3` and emits the call against the existing C symbol `print_at` (unchanged in `src/lib/print_at.c`). The Rock-level `register_builtin("print_at", …)` line was removed; the component's files, wiki page, and C symbol keep the historical name for grep/debug continuity. `test/print_at_test.rkr` migrated to `print(x, y, text)`. Verified: 229/229 host tests pass, ZXN build of `print_at_test.nex` clean, generated C shows `print_at(...)` call emission as expected. The overloading TODO on the rtl-print-at page is now resolved; only the raster-replacement TODO remains open.

## [2026-04-13] todo | RTL colour component (ink / paper / bright / flash) — plan drafted

Drafted `pages/rtl/rtl-ink-paper.md` (status: draft) covering the planned ZX character-cell attribute component: `ink`, `paper`, `bright`, `flash`, `inverse`, `over`. ZXN path uses ROM channel #2 control codes 16-21 via the same `rst 0x10` wrapper `print_at` already uses; host path keeps a small termbox2 attribute shadow behind a new `host_caps.ink` flag and `print_at.c`'s `tb_print` call reads the shadow instead of `TB_DEFAULT`. On ZXN no change to `print_at` is needed — the ROM already applies current attributes to subsequent `RST 10h` output. Two open design questions recorded on the page: (a) whether to lift the `rst10_emit` scratch byte + wrapper out of `print_at.c` into a shared `src/lib/zxn/rom_channel2.{h,c}` (mildly revising rule 1 of rtl-overview to allow shared lifecycle helpers) — recommendation: lift; (b) individual functions vs a single `set_attr(kind, val)` — recommendation: individual. No code yet.

## [2026-04-13] update | ink/paper plan revised (ownership + DRY, no extracted helpers)

Reworked `pages/rtl/rtl-ink-paper.md` after architectural pushback: (a) dropped the `zx_putchar` extraction — it was encoding a ROM-era coincidence that both "render character" and "set attribute" happen to funnel through `RST 10h`. In the raster era they share no code at all. Both files keep a private inline-asm wrapper; duplication is deliberate and vanishes at raster-migration time. (b) dropped the proposed `host/attr_state` helper file — the attribute state is owned by `ink_paper`, full stop. `ink_paper.h` exposes `attr_fg()` / `attr_bg()` accessors; `print_at.c`'s host branch reads them. (c) rule 1 gains a one-sentence clarification that consuming a sibling component's public header is legitimate API use, not a cross-component include in the bad sense. DRY inside `ink_paper.c` is handled by one internal `set_attr(kind, val)` helper that all six public setters call. No external helper files created. Memory layout (pinning state vars to a specific address) deferred to a future ZXN micro-management pass.

## [2026-04-13] update | ink_paper RTL component shipped
Implemented `ink`/`paper`/`bright`/`flash`/`inverse`/`over` as Rock builtins.
`src/lib/ink_paper.{h,c}` owns attribute state; setters DRY through an internal
`set_attr` helper. ZXN branch emits two-byte control-code sequences via a
private RST 10h wrapper (deliberately duplicated from print_at.c until raster
migration). Host branch shadows state and exposes `attr_fg()`/`attr_bg()`
composing Spectrum palette → termbox2 (with ZX-to-ANSI colour lookup), which
print_at.c now reads instead of passing TB_DEFAULT. `host_caps.ink` flag added.
All 230 host tests pass (+5 new); `./rock --target=zxn test/ink_paper_test.rkr`
produces a clean `.nex`. rtl-overview rule 1 reworded around DRY with the
public-header consumption clarification. Page flipped to status: current.

## [2026-04-13] update | rtl-input page added; keyboard split documented
Created `wikiroot/pages/rtl/rtl-input.md` for `inkey`/`keypress` (ROM route
via LAST_K + FLAGS bit 5). Updated `rtl-keyboard.md` with a "When to use this
vs rtl-input" table clarifying that the matrix scanner is for action games
needing simultaneous key-holds, while the ROM route is for menus/text.
Rock ships both components deliberately — they serve different needs and can
be used together. Dependencies and limitations documented on both pages.
Added both pages to index.md under Rock RTL.

## [2026-04-15] lint | rewrite subdir wiki-links to full keys

Changed files: 6

Automated tool: exit 0 for links; 107 broken links resolved
Issues fixed: rewrote [[rtl-X]] → [[rtl/rtl-X]], [[pasta80-X]] → [[pasta80/pasta80-X]], [[zxn-ula]] → [[targets/zxn/zxn-ula]] across 18 files in pasta80/ and rtl/
Remaining issues: pending source wikiroot/new/keyboard_example (awaits /wiki ingest)
TODOs filed: none


## [2026-04-16] update | document Rock operator gotchas

Changed files: 2

Added "Operators Rock does NOT have" table to control-flow.md covering
==, !, and/or keywords, ternary, compound assignment, and ++/--.
Collected during pasta80 RTL parity work when writing test fixtures.

