---
title: "PASTA/80 — Project Overview"
category: pasta80
tags: [pasta80, pascal, z80, cross-compiler, rtl]
sources: [pasta80]
updated: 2026-04-12
status: current
---

# PASTA/80 — Project Overview

PASTA/80 is a **Pascal cross-compiler** targeting the Z80 microprocessor, written in Free Pascal. It compiles a dialect closely matching Turbo Pascal 3.0 and generates Z80 assembly, using **sjasmplus** as the assembler backend.

## Relevance to Rock

PASTA/80 is a sibling project to Rock in spirit: both are single-pass compilers targeting the ZX Spectrum Next (among other platforms), both need a cross-platform runtime library, and both face the same Z80 memory constraints. Studying PASTA/80's RTL gives Rock a head start on:

- **Cross-platform RTL architecture** — how to share code across targets
- **HAL (Hardware Abstraction Layer) design** — platform-specific I/O behind a stable API
- **Mixed high-level/assembly implementation** — what belongs in C/Pascal vs assembly
- **Memory-constrained library design** — keeping binary size small on 64K machines

## Compiler Architecture

| Aspect | Detail |
|--------|--------|
| **Source language** | Pascal (Turbo Pascal 3.0 dialect with extensions) |
| **Host compiler** | Free Pascal (fpc) |
| **Approach** | Single-pass recursive-descent, no explicit AST |
| **Code generation** | Emits Z80 assembly directly during parsing |
| **Assembler backend** | sjasmplus |
| **Optimisations** | Peephole (`--opt`), dead-code elimination (`--dep`) |

## Target Platforms

| Target | Flag | Output | File I/O | Overlays |
|--------|------|--------|----------|----------|
| **CP/M** | *(default)* | `.com` | Yes (BDOS) | No |
| **ZX Spectrum 48K** | `--zx48` | `.bin`, `.sna`, `.tap` | No | No |
| **ZX Spectrum 128K** | `--zx128` | `.bin`, `.sna`, `.tap` | No | Yes (bank switching) |
| **ZX Spectrum Next** | `--zxnext` | `.bin`, `.tap`, `.run` | Yes (esxDOS) | Yes (page mapping) |
| **Agon Light** | `--agon` | MOS binary | Yes (MOS API) | Yes (memory copy) |

## Key Features

- **Full Turbo Pascal 3.0 coverage**: all basic types, records, sets, enumerations, pointers, files (3 kinds), heap management, inline assembly
- **Extensions beyond TP3**: conditional compilation (`{$ifdef}`), `Break`/`Continue`, `Inc`/`Dec`, colour support, assertions, breakpoints, overlays via bank switching
- **Overlay system**: Procedures marked `overlay` are placed in switchable 8K memory banks, transparent to the caller. Managed entirely by the compiler.
- **Mini IDE**: Terminal-based Turbo Pascal-style IDE (`--ide` flag)

## Compilation Pipeline

```
Pascal source (.pas)
    |
    v
PASTA/80 compiler (single-pass recursive descent)
    |
    v
Z80 assembly (.asm) + RTL assembly includes
    |
    v
sjasmplus assembler
    |
    v
Binary (.com / .bin / .sna / .tap / .nex / MOS)
```

The RTL is compiled into the binary — there is no separate linking step. The compiler `{$i}`-includes RTL Pascal files and `{$l}`-includes RTL assembly files, so the entire RTL is assembled together with user code.

## Project Structure

```
pasta.pas           Main compiler source (Free Pascal)
rtl/                Runtime library (see [[pasta80-rtl-architecture]])
  system.pas        Core RTL: types, heap, math, strings, constants
  system.asm        Core Z80 assembly: arithmetic, string ops, heap, sets
  math48.asm        6-byte floating-point math (Hejlsberg)
  cpm.pas           CP/M target: VT52 terminal, BDOS, files, command line
  cpm.asm           CP/M target: putc, readkey, readline, init/shutdown
  zx.pas            ZX 48K entry point (includes system + zxrom)
  zxrom.pas         ZX Spectrum: screen, keyboard, sound, graphics, timing
  zxrom.asm         ZX Spectrum: ROM calls, attribute handling, plot/draw
  zx128.pas         ZX 128K entry point (includes system + zxrom + derby)
  derby.pas/asm     128K bank selection
  next.pas          Next entry point (includes system + zxrom + esxdos + files + tbblue)
  esxdos.pas/asm    Next: esxDOS file system calls
  tbblue.pas/asm    Next: register access, CPU speed, memory paging
  agon.pas          Agon entry point (includes system + agon-specific)
  agon.asm          Agon: VDP output, keyboard, MOS API, graphics
  agonhead.asm      Agon: MOS header and startup
  agonover.asm      Agon: overlay loading
  files.pas         Shared file abstraction (text files + typed files)
  overlays.asm      Overlay dispatch
  helpers.lua       sjasmplus Lua macros
examples/           Sample programs
tests/              Test suite (runner.pas, all.pas)
docs/               Documentation including RTL reference
misc/               Config, BASIC loaders, release scripts
```

## See Also

- [[pasta80-rtl-architecture]] — Detailed RTL layering and HAL design
- [[pasta80-rtl-api]] — Full RTL API reference
- [[pasta80-target-platforms]] — Per-platform implementation details
- [[pasta80-lessons-for-rock]] — Patterns and lessons applicable to Rock's RTL
