# Rock Wiki — Content Index

Organised by category. Each entry: `page-name — one-line description`.

---

## Overview

- [[overview]] — High-level architecture synthesis: pipeline, components, targets, memory model
- [[glossary]] — Canonical domain glossary (alphabetical)
- [[domain-model]] — Bounded context map with Mermaid diagram and integration table

---

## Lexer

- [[lexer-overview]] — Token types, scanning algorithm, escape handling, data structures

---

## Parser

- [[parser-overview]] — Grammar rules, AST node types, precedence climbing, include splicing

---

## Generator

- [[generator-overview]] — AST → C emission, pre_f buffer, type-specific wrapper synthesis, builtins

---

## Targets

- [[targets/host-gcc]] — Host target: gcc compilation, absolute includes, test coverage
- [[targets/zxn-z80]] — ZXN target: Z88DK/SDCC, memory layout, statement splitting, Z80 interop
- [[targets/zxn-hardware]] — ZXN hardware overview: layer stack, port access patterns, subsystem index
- [[targets/zxn/zx-z88dk-startupcrt-summary]] — Z88DK startup CRT profiles and output-driver implications
- [[targets/zxn/zxn-sample-programs]] — ZXN sample-program hub linking concrete sjasmplus code to hardware pages

### Z88DK Tools and Assembly

- [[targets/zxn/tools/z88dk-inline-asm]] — `#asm`/`#endasm` syntax, calling conventions, data types, return registers
- [[targets/zxn/tools/z88dk-z80-library]] — `<z80.h>`: timing delays, port I/O, IM2 interrupt setup, memory access macros
- [[targets/zxn/tools/z80asm-reference]] — z80asm assembler/linker: input format, preprocessor, expressions, all directives

### ZXN Hardware Subsystems

- [[targets/zxn/zxn-ports-registers]] — Complete I/O port map and Next register index (all 130 registers)
- [[targets/zxn/zxn-memory-paging]] — 8K/16K bank paging, 8 MMU slots, legacy 128K/+3 modes
- [[targets/zxn/zxn-dma]] — zxnDMA programming: WR0–WR6, fixed-time transfer, examples
- [[targets/zxn/zxn-palette]] — 256-entry palettes, 8-bit/9-bit colour, Layer 2 priority flag
- [[targets/zxn/zxn-ula]] — Classic 256×192 pixel display, attributes, border, shadow screen
- [[targets/zxn/zxn-layer2]] — Full-colour framebuffer: 256×192/320×256/640×256 modes, bank paging
- [[targets/zxn/zxn-tilemap]] — 8×8 tile block display: 40×32/80×32, tile definitions, stencil mode
- [[targets/zxn/zxn-sprites]] — 128 hardware sprites: patterns, palette offset, composite/unified groups
- [[targets/zxn/zxn-copper]] — Raster co-processor: WAIT/MOVE/HALT/NOOP, program upload
- [[targets/zxn/zxn-sound]] — 3× AY-3-8912 (Turbo Sound), 9 channels, envelope, DAC
- [[targets/zxn/zxn-keyboard]] — 8×5 key matrix (port `$xxFE`) and 10 extended keys (`$B0`/`$B1`)
- [[targets/zxn/zxn-interrupts]] — IM1/IM2/Hardware IM2, 14 vectored sources, line interrupt, CTC

### ZXN Sample Program Walkthroughs

- [[targets/zxn/samples/zxn-interrupt-samples]] — Comparison of IM1, legacy IM2, safe IM2, and Hardware IM2 setup samples
- [[targets/zxn/samples/zxn-im1-sample-summary]] — IM1 sample using bank paging to place a handler at `$0038`
- [[targets/zxn/samples/zxn-im2-sample-summary]] — Legacy IM2 sample using a 128-entry vector table
- [[targets/zxn/samples/zxn-im2hw-sample-summary]] — Hardware IM2 sample using the Next vectored interrupt registers
- [[targets/zxn/samples/zxn-im2safe-sample-summary]] — Bus-safe IM2 sample with a 257-byte table and `$F0F0` handler
- [[targets/zxn/samples/zxn-sprite-sample-summary]] — Sprite sample using DMA pattern upload and unified relative groups
- [[targets/zxn/samples/zxn-sound-sample-summary]] — Sound sample using Turbo Sound setup and AY register playback
- [[targets/zxn/samples/zxn-tilemap-sample-summary]] — Tilemap sample using map/tile/palette assets and offset shake
- [[targets/zxn/samples/zxn-layer2-samples]] — Comparison of 256x192, 320x256, and 640x256 Layer 2 drawing samples
- [[targets/zxn/samples/zxn-layer2-256x192-sample-summary]] — Layer 2 256x192 sample using six 8K row banks
- [[targets/zxn/samples/zxn-layer2-320x256-sample-summary]] — Layer 2 320x256 sample using ten 8K column banks
- [[targets/zxn/samples/zxn-layer2-640x256-sample-summary]] — Layer 2 640x256 sample using packed 4bpp byte columns
- [[targets/zxn/samples/zxn-copper-sample-summary]] — Copper sample using list upload, live instruction patching, and Layer 2 palette/clip effects

---

## Testing

- [[testing/testing-overview]] — Test harness (Assert.rkr), 27-test suite table, test lifecycle

---

## Syntax

- [[syntax/syntax-index]] — Keyword and built-in function quick reference
- [[syntax/comments-and-includes]] — Comment forms, include splicing, and module requirement
- [[syntax/types]] — Scalar types, arrays, variable declaration styles, default values, conversions
- [[syntax/control-flow]] — if/then/else, while, for counter loop, for-in iterator, match, operators
- [[syntax/functions-and-methods]] — sub, parameters, return, method syntax, name mangling
- [[syntax/arrays]] — Declaration, append/get/set/pop/insert/length, iteration, fixed vs dynamic
- [[syntax/strings]] — concat, substring, toString, charAt, equals, print, char type
- [[syntax/builtins-and-io]] — Output, file I/O, command-line args, conversions, peek/poke
- [[syntax/modules-and-records]] — record, enum (simple + tagged union), module singleton
- [[syntax/embed]] — @embed c / @embed asm inline blocks

---

## PASTA/80 (Reference Project)

- [[pasta80-overview]] — Project overview: Pascal cross-compiler for Z80, RTL design, target platforms
- [[pasta80-rtl-architecture]] — 4-layer RTL architecture, Block* HAL pattern, mixed Pascal/ASM strategy
- [[pasta80-rtl-api]] — Full RTL API reference (~115 symbols) organised by category and platform
- [[pasta80-target-platforms]] — CP/M, ZX48, ZX128, Next, Agon: composition, FCB design, platform APIs
- [[pasta80-lessons-for-rock]] — Patterns to adopt, pitfalls to improve on, Rock RTL strategy recommendations

---

## Rock RTL

- [[rtl-overview]] — Cross-platform RTL strategy, component conventions, SDCC symbol rules
- [[rtl-keyboard]] — Matrix scanner for simultaneous key-holds (action games; ZXN + host stub)
- [[rtl-input]] — ASCII `inkey`/`keypress` via ROM key-scan (menus, text; ZXN + termbox2)
- [[rtl-border]] — Second component: border colour via `<z80.h>` port I/O (C-only, no asm file)
- [[rtl-print-at]] — Third component: positioned text via ROM RST 10h (pasta80-style; raster replacement planned)
- [[rtl-host-caps]] — Centralised host capability + lifecycle layer (rule 11; runs once at program startup)
- [[rtl-ink-paper]] — `ink`/`paper`/`bright`/`flash`/`inverse`/`over` via ROM channel #2 control codes
- [[rtl-plot]] — Raster pixel `plot(x,y)` via Z80N PIXELAD/SETAE (host: termbox2 quadrant blocks)
- [[rtl-draw]] — Raster line `draw(x0,y0,x1,y1)` with H/V fast paths + shallow/steep Bresenham
- [[rtl-polyline]] — Connected segments `polyline(byte[] xs, byte[] ys)` — thin wrapper over `draw`
- [[rtl-circle]] — Circle outline `circle(cx, cy, r)` via integer midpoint algorithm with 8-way symmetry
- [[rtl-fill]] — Rectangle fill `fill_rect(x0,y0,x1,y1)` delegating rows to `draw`'s H fast path
- [[rtl-triangle]] — Triangle outline + scanline-rasterised `filled_triangle` with 16-bit-safe edge interpolation
- [[rtl-cls]] — `cls()` — clear screen via ROM CLS on ZXN, termbox2 on host
- [[rtl-sound]] — `beep(freq, dur)` — square-wave tone via ROM BEEPER on ZXN, terminal bell on host
- [[rtl-time]] — `sleep(ms)` — delay via z88dk `z80_delay_ms` on ZXN, `usleep` on host
- [[rtl-random]] — `randomize`/`random_byte`/`random_word` — 16-bit LCG, R-register seed on ZXN
- [[rtl-nextreg]] — `next_reg_set/get`, `cpu_speed_set/get`, `mmu_set` — ZXN Next register access
- [[rtl-helpers]] — `odd`/`even`/`hi`/`lo`/`swap`/`upcase`/`locase`/`abs_int`/`abs_word` — scalar utilities
- [[rtl-fmath]] — `float` type + `fsin`/`fcos`/`fsqrt`/`fabs_float`/`fpi` — float math

---

## Decisions

- [[decisions/ADR-0001-function-overloading-arity-only]] — Arity-based overloading shipped; type-based deferred to Phase 3
- [[decisions/ADR-0002-string-view-memory-model]] — *Superseded by ADR-0003.* Original draft of immutable string views.
- [[decisions/ADR-0003-memory-model]] — Draft plan: named fixed-bounds pools, block-scoped regions, refcounted handles, descriptor-with-capacity strings, escape-driven implicit promotion

---

## Concepts

- [[concepts/compilation-pipeline]] — CLI args, step-by-step pipeline, include resolution, memory lifecycle
- [[concepts/name-table]] — Symbol table design, scope model, type inference role
- [[concepts/string-representation]] — rock_string struct, temporaries, runtime ops, deep copy
- [[concepts/array-internals]] — __internal_dynamic_array_t, generic ops, type-specific wrappers, growth strategy
