---
title: "RTL Component: print (positioned)"
category: concepts
tags: [rtl, print, text, zxn, rom, rst10, builtins, overloading]
sources: [src/lib/print_at.h, src/lib/print_at.c, src/lib/host_caps.h, src/lib/host_caps.c, src/lib/host/termbox2_impl.c, src/ext/lib/termbox2.h, test/print_at_test.rkr]
updated: 2026-04-13
status: current
---

# RTL Component: print (positioned overload)

Third RTL component under the [[rtl/rtl-overview]] conventions. Draws a Rock string at a character cell `(x, y)` on the ZX upper screen by calling ROM `RST 10h` directly, preceded by the `AT` control sequence. The ZXN implementation is a direct port of pasta80's approach (see [[pasta80/pasta80-target-platforms]] §ZX Spectrum 48K).

## Rock-facing API

The positioned form is a 3-arg **overload of `print`**, landed via Phase 2 of the overloading work ([[decisions/ADR-0001-function-overloading-arity-only]]):

```rock
print("hello");                              // 1-arg form: writes to stdout / channel #2
print(to_byte(10), to_byte(5), "hello");     // 3-arg form: positioned
```

| Call shape | Return | Args | Purpose |
|------------|--------|------|---------|
| `print(text)`         | `void` | `string text`                 | Write `text` at the current cursor. |
| `print(x, y, text)`   | `void` | `byte x, byte y, string text` | Draw `text` starting at column `x`, row `y` (both 0-based). |

`print/3` is not mangled: the arity-based mangling scheme only kicks in when ≥2 **user** fundefs share a name (see [[generator/generator-overview]] §Function Overloading). The `print` overload is handled by a dedicated fast-path branch at the top of `generate_funcall()` in `src/generator.c` — similar in shape to the existing `append`/`get`/`concat`/`printf` fast-paths — that recognises `print` with 3 arguments and routes the emission to the C symbol `print_at` in `src/lib/print_at.c`. The C function name is unchanged for ease of grep and debugging; only the Rock-facing name is unified under `print`.

The Rock name `print_at` is **retired** (no longer registered as a builtin). Rock source must use `print(x, y, text)`. The `src/lib/print_at.{h,c}` filenames, the C symbol `print_at`, and the wiki page title keep the historical name for continuity.

Coordinates use the standard 32×24 upper-screen layout: column 0..31, row 0..23. Out-of-range coordinates pass straight through to the ROM; the ROM will clip or wrap per its own rules.

## ZXN implementation: raw ROM RST 10h

This component deliberately **bypasses** the z88dk CRT's control-code-handling layer. Instead it calls ROM `RST 10h` directly with three preamble bytes and one byte per character:

```
22 (AT), row, col, text[0], text[1], ..., text[length-1]
```

The ROM upper-screen channel (channel #2, opened at CRT startup with `-startup=1`) interprets the `AT` control code natively, updating its cursor sysvars before printing the rest of the bytes.

Each byte is emitted via a tiny inline-asm wrapper that reads from a shared file-scope scratch variable:

```c
unsigned char rock_zx_rom_byte;   /* non-static so inline asm can reach it */

static void rst10_emit(void) {
  __asm
    ld a, (_rock_zx_rom_byte)
    rst 0x10
  __endasm;
}
```

The scratch-variable approach sidesteps any argument-passing subtleties of SDCC's `sdcc_iy` calling convention — there is nothing to get wrong because the wrapper takes zero arguments. `print_at` writes the byte, calls `rst10_emit`, repeat.

## Host implementation: termbox2 with stdout fallback

The host path uses the vendored **termbox2** library (single-header, at `src/ext/lib/termbox2.h`) to render into a terminal cell grid that matches the ZX's 32×24 layout. Rock programs built with `--target=gcc` can then be previewed in a real terminal with approximately the same placement they would have on a Next.

`src/lib/host/termbox2_impl.c` is a dedicated one-file translation unit that defines `TB_IMPL` and includes `termbox2.h`, so the implementation is compiled exactly once. Other host RTL components can just `#include "termbox2.h"` without worrying about multiple-definition errors. The file wraps its include in `#pragma GCC diagnostic ignored "-Wunused-function"` because termbox2 ships static helpers that the library's own configurations sometimes leave unused.

`print_at.c`'s host branch is now a flat capability check — all lifecycle (isatty probe, `tb_init`, `atexit` teardown) lives in [[rtl/rtl-host-caps]] (`src/lib/host_caps.c`) and runs **once** at program startup from the generator-emitted `main()` wrapper. `print_at` only reads `host_caps.print_at`:

1. If `host_caps.print_at` is set, copy the Rock string into a 256-byte null-terminated stack buffer (termbox2 wants a C string), hand it to `tb_print(x, y, TB_DEFAULT, TB_DEFAULT, buf)`, then `tb_present()`.
2. Otherwise (no tty, or `tb_init` failed at startup) fall back to the plain-text `@(x,y) text\n` form. The test harness keeps working because `run_tests.sh` pipes stdout — the capability stays off.

The fallback branch is the same simple line-oriented form Rock had before termbox2 was wired up, so `./run_tests.sh` still sees deterministic text output.

> **Caveat:** On most terminals termbox2 switches to the alternate screen buffer, so the rendered grid disappears when the program exits. A Rock program that calls `print_at` and then terminates immediately will flash the output for a single frame. A future `wait_key()` or event-loop builtin will give programs a natural way to block for inspection; until then, manual verification requires a program that loops.

## TODOs

> **TODO:** **Replace ROM path with framebuffer rasterisation.** The ROM route only works while the Spectrum ROM is paged in at `$0000-$3FFF`. When Rock programs start swapping the ROM out (for RAM banks) or using custom character fonts, `print_at` must rasterise glyph bytes directly into the ULA framebuffer at `$4000-$57FF` using the eccentric Y-bit permutation: `addr = 0x4000 | ((r & 0x18) << 5) | ((r & 0x07) << 5) | (c & 0x1F)`, stepping 8 scanlines by `+0x100`. A future component (probably `src/lib/zxn/raster.asm`) will own this. When it lands, `print_at` should switch to the raster path and this component retired or demoted to a fallback.

~~**Builtin overloading.** The user-facing name is `print_at` rather than overloading `print` because Rock's current call dispatch is name-only.~~ **Resolved 2026-04-13** — arity-based overloading shipped as Phase 1, and this component was re-exposed as the 3-arg overload of `print` in Phase 2. See [[decisions/ADR-0001-function-overloading-arity-only]].

The raster TODO remains open and is filed in `wikiroot/log.md` with type `todo`.

## Implementation layout

| File | Purpose |
|------|---------|
| `src/lib/print_at.h` | Declaration + feature doc + TODO markers |
| `src/lib/print_at.c` | ZXN: scratch byte + inline asm `rst 0x10`; host: termbox2 with stdout fallback |
| `src/lib/host/termbox2_impl.c` | Single-TU host-only file defining `TB_IMPL` for termbox2 (shared by any future host component that needs termbox2) |
| `src/ext/lib/termbox2.h` | Vendored termbox2 single header |
| `test/print_at_test.rkr` | Smoke test — top-left, middle, bottom-right, empty string |

No separate `.asm` file and no exported asm symbols. Inline asm inside the C shim is sufficient here; this is a stronger case than [[rtl/rtl-border]] because we need to call a ROM vector, which inline asm handles fine.

## Build integration

`rock` now carries a `RTL_HOST_SRCS` list alongside `RTL_CORE_SRCS` / `RTL_C_SRCS` / `RTL_ZXN_ASM`. Host-only TUs go here; the gcc branch appends them to the compile line and adds `-I src/ext/lib`. The SDCC branch ignores them, so termbox2 never reaches the ZXN build. This is the new recipe for any host-only helper:

```bash
RTL_HOST_SRCS="\
  $RT/lib/host/termbox2_impl.c"
```

Adding a future host helper (say `host/sdl_impl.c`) is another one-line append plus any needed `-I` path.

## Verification

- `./run_tests.sh` — stdout is piped, so the fallback branch is exercised; `print_at_test` contributes 1 smoke assertion and total 225 host tests pass. Zero warnings after the `-Wunused-function` pragma.
- `rock test/print_at_test.rkr && ./test/print_at_test.exe` in a real terminal — termbox2 initialises, `tb_print` draws into cell positions `(0,0)`, `(10,5)`, `(31,23)`, then `tb_shutdown` runs on program exit.
- `rock --target=zxn test/print_at_test.rkr` — produces `print_at_test.nex` (~33 KB) cleanly on first try; no SDCC errors, only pre-existing `Assert.rkr` unused-arg warnings. termbox2 is never included in the SDCC translation unit because of the `#ifdef __SDCC` split.
- Manual hardware check (required for raster migration later): load `.nex` in CSpect / on a real Next, verify that `"top left"` appears at column 0 row 0, `"middle"` at column 10 row 5, and `"br"` at column 31 row 23.

## Why not z88dk `printf` / `putchar` with control codes?

z88dk's `+zxn` CRT with `-startup=1` *does* support control-code-driven positioning through its stdio layer. We chose not to rely on it because:

1. **Decoupling from CRT internals.** The stdio control-code handler is a z88dk convenience layer, not a ZX hardware feature. Calling ROM directly keeps `print_at` one step closer to the metal and independent of startup-profile changes.
2. **Footprint.** `printf` pulls in formatted-output support; we only need byte-copying.
3. **Preparation for the raster replacement.** When the raster version lands we already own the output loop — there is no stdio call site to refactor.

## See Also

- [[rtl/rtl-overview]] — conventions and the SDCC underscore-prefix gotcha
- [[rtl/rtl-keyboard]], [[rtl/rtl-border]] — prior components
- [[pasta80/pasta80-target-platforms]] — pasta80's ZX48/Next ROM approach (the model)
- [[targets/zxn/zxn-ula]] — ULA framebuffer layout (for the future raster implementation)
