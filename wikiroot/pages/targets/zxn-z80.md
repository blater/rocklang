---
title: ZXN Target (ZX Spectrum Next / Z80)
category: targets
tags: [zxn, z80, spectrum-next, z88dk, sdcc, embedded, target]
sources: [zx-z88dk-startupcrt.md]
updated: 2026-04-11
status: current
---

# ZXN Target (ZX Spectrum Next / Z80)

The **ZXN target** compiles Rock programs to `.nex` executables for the ZX Spectrum Next (Z80 architecture) using the Z88DK toolchain with the SDCC backend.

## Toolchain

| Tool | Role |
|------|------|
| `rock --target=zxn` | Driver script: runs `rockc`, then invokes `zcc` to build a `.nex` |
| `zcc +zxn -clib=sdcc_iy` | C → Z80 `.nex` via SDCC |
| `rockc` | Rock → C transpiler binary |

## Invocation

```bash
rock input.rkr output --target=zxn
```

The normal entry point is `rock`, which emits `output.c`, builds the ZXN binary, then reports `output.nex`.
`rockc` can be invoked directly when you want the transpile step only.

## Z88DK / SDCC Compiler Selection

Z88DK's `zcc` frontend can use either `sccz80` or `SDCC` as the backend. Rock uses **SDCC** (via `-clib=sdcc_iy`) because:
- SDCC handles struct return values correctly (required by `fundefs_internal.c`)
- `sccz80` crashed on struct returns in earlier versions

## Startup CRT and Output Drivers

Z88DK's **startup CRT** is selected with `-startup=n` and controls the runtime setup before `main()`, including which standard I/O drivers are present. Rock currently uses `-startup=1`, which corresponds to a 32x24 text output driver with control-code handling in the Z88DK startup notes.

Other startup values trade capability for size: `-startup=0` uses 32x24 output without control codes, `-startup=4`/`5` select 64x24 fixed-width output, `-startup=8`/`9` select FZX proportional output, and `-startup=31` removes stdin/stdout/stderr for minimal programs. See [[targets/zxn/zx-z88dk-startupcrt-summary]].

## Include Strategy

The generator emits **relative runtime header names**:
```c
#include "alloc.h"
#include "fundefs_internal.h"
```

The `rock` driver invokes `zcc` with `-I"$ROCK_ROOT/src/lib"` so those headers resolve against the runtime library directory.

## Compilation Command

```bash
zcc +zxn -vn -subtype=nex -startup=1 -clib=sdcc_iy -create-app \
    -pragma-include:"$ROCK_ROOT/src/lib/zxn/zpragma_zxn.inc" \
    -I"$ROCK_ROOT/src/lib" \
    output.c \
    "$ROCK_ROOT/src/lib/fundefs_internal.c" \
    "$ROCK_ROOT/src/lib/alloc.c" \
    "$ROCK_ROOT/src/lib/asm_interop.c" \
    "$ROCK_ROOT/src/lib/fundefs.c" \
    -o output
```

## Memory Layout (zpragma_zxn.inc)

The `zpragma_zxn.inc` file configures memory bank mapping for the ZX Spectrum Next. It is included via `#pragma-include` and controls:
- Stack placement
- Heap placement
- Bank configuration

The `rock` driver passes the pragma file path directly to `zcc`.

## Statement Splitting (pre_f)

SDCC is stricter about C syntax than gcc. Complex inline expressions (particularly string literal construction) must be split into separate statements.

The generator's `pre_f` buffer accumulates setup statements that precede the expression using them. Before each statement is emitted to `f`, the `pre_f` buffer is flushed first.

Example: `print(concat(a, b))` generates:
```c
// pre_f:
rock_string __strtmp_0 = __concat_str(a, b);
// f:
print(__strtmp_0);
```

## Z80 Assembly Interop

Rock supports inline Z80 assembly via embed blocks:
```rock
@embed asm
  ld a, 42
  out (0xfe), a
@end asm
```

The `asm_interop.h` / `asm_interop.c` files in `src/lib/` provide C-callable wrappers for common Z80 I/O operations (port reads, writes, memory-mapped access). See [[syntax/embed]] for embed block syntax.

## peek / poke

The Rock built-in functions `peek(addr)` and `poke(addr, val)` are especially useful on ZXN for direct memory-mapped I/O:
```rock
poke(0x5c00, 42);     // Write to ZX Spectrum memory
byte val := peek(0x5c00);
```

These map directly to C pointer casts: `*(uint8_t*)(addr)`.

## Z88DK Tools and Assembly

Detailed reference documentation for the Z88DK toolchain used by the ZXN target:

- [[targets/zxn/tools/z88dk-inline-asm]] — `#asm`/`#endasm` syntax, calling conventions, data types, return registers
- [[targets/zxn/tools/z88dk-z80-library]] — `<z80.h>` library: timing delays, port I/O, IM2 interrupt setup, memory access
- [[targets/zxn/tools/z80asm-reference]] — z80asm assembler/linker: input format, expressions, preprocessor macros, all directives, command line

## ZXN Hardware Reference

The Next hardware is documented in a dedicated hierarchy under `pages/targets/zxn/`:

- [[targets/zxn-hardware]] — overview, layer compositing, port access patterns
  - [[targets/zxn/zxn-ports-registers]] — complete port and Next register index
  - [[targets/zxn/zxn-memory-paging]] — 8K/16K bank paging, MMU slots (`$50`–`$57`)
  - [[targets/zxn/zxn-dma]] — bulk DMA transfers, fixed-time audio streaming
  - [[targets/zxn/zxn-palette]] — 8-bit/9-bit colour palettes, shared by all layers
  - [[targets/zxn/zxn-ula]] — classic 256×192 display with attributes
  - [[targets/zxn/zxn-layer2]] — full-colour framebuffer (256×192, 320×256, 640×256)
  - [[targets/zxn/zxn-tilemap]] — 8×8 tile block display (40×32 or 80×32)
  - [[targets/zxn/zxn-sprites]] — 128 hardware sprites with anchor/relative groups
  - [[targets/zxn/zxn-copper]] — raster co-processor for per-scanline effects
  - [[targets/zxn/zxn-sound]] — 3× AY-3-8912 sound chips (Turbo Sound)
  - [[targets/zxn/zxn-keyboard]] — 8×5 key matrix + 10 extended keys
  - [[targets/zxn/zxn-interrupts]] — IM1/IM2/Hardware IM2 with 14 vectored sources

## Known Limitations

| Feature | Status |
|---------|--------|
| `enum_test.rkr` | Currently fails on ZXN due to SDCC enum syntax incompatibility |
| Inline assembly | Supported via `@embed asm`; generator emits `#asm`/`#endasm` (Z88DK preprocessor directives, processed by `zcc` before SDCC); host builds skip the block via `#ifdef __SDCC` |
| Dynamic memory | Arena allocator works; SDCC heap is limited on Z80 |

## Verified Working

The following tests have been verified to produce valid `.nex` files on ZXN:
- `test/simple_test.rkr`
- `test/array_test.rkr`
- `test/concat_test.rkr`
- `test/byte_test.rkr`
- `test/format_test.rkr`

See [[testing/testing-overview]] for the full test suite, and [[targets/host-gcc]] for the host target.
