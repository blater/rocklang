# Rock Wiki — Content Index

Organised by category. Each entry: `page-name — one-line description`.

---

## Overview

- [[overview]] — High-level architecture synthesis: pipeline, components, targets, memory model
- [[ubiquitous-language]] — Canonical domain glossary (alphabetical)
- [[domain-model]] — Bounded context map with Mermaid diagram and integration table

---

## Lexer

- [[lexer/lexer-overview]] — Token types, scanning algorithm, escape handling, data structures

---

## Parser

- [[parser/parser-overview]] — Grammar rules, AST node types, precedence climbing, include splicing

---

## Generator

- [[generator/generator-overview]] — AST → C emission, pre_f buffer, type-specific wrapper synthesis, builtins

---

## Targets

- [[targets/host-gcc]] — Host target: gcc compilation, absolute includes, test coverage
- [[targets/zxn-z80]] — ZXN target: Z88DK/SDCC, memory layout, statement splitting, Z80 interop
- [[targets/zxn-hardware]] — ZXN hardware overview: layer stack, port access patterns, subsystem index
- [[targets/zxn/zxn-sample-programs]] — ZXN sample-program hub linking concrete sjasmplus code to hardware pages

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

---

## Testing

- [[testing/testing-overview]] — Test harness (Assert.rkr), 27-test suite table, test lifecycle

---

## Syntax

- [[syntax/syntax-index]] — Keyword and built-in function quick reference
- [[syntax/types]] — Scalar types, arrays, variable declaration styles, default values, conversions
- [[syntax/control-flow]] — if/then/else, while, for counter loop, for-in iterator, match, operators
- [[syntax/functions-and-methods]] — sub, parameters, return, method syntax, name mangling
- [[syntax/arrays]] — Declaration, append/get/set/pop/insert/length, iteration, fixed vs dynamic
- [[syntax/strings]] — concat, substring, to_string, get_nth_char, print, char type
- [[syntax/modules-and-records]] — record, pro (product type), enum, module singleton
- [[syntax/embed]] — @embed c / @embed asm inline blocks

---

## Concepts

- [[concepts/compilation-pipeline]] — CLI args, step-by-step pipeline, include resolution, memory lifecycle
- [[concepts/name-table]] — Symbol table design, scope model, type inference role
- [[concepts/string-representation]] — rock_string struct, temporaries, runtime ops, deep copy
- [[concepts/array-internals]] — __internal_dynamic_array_t, generic ops, type-specific wrappers, growth strategy
