---
title: Rock Transpiler — Architecture Overview
category: overview
tags: [architecture, transpiler, pipeline, host, zxn]
sources: [samples/im1/main.asm, samples/im2/main.asm, samples/im2hw/main.asm, samples/im2safe/main.asm]
updated: 2026-04-11
status: current
---

# Rock Transpiler — Architecture Overview

Rock is a statically-typed, C-transpiling language targeting two platforms: **host** (standard C via gcc) and **ZXN** (ZX Spectrum Next via Z88DK/SDCC for Z80). The compiler is written in C and follows a classical three-phase pipeline.

## Compilation Pipeline

```
source.rkr
    │
    ▼
┌─────────┐
│  Lexer  │  lexer.c / token.c   — character stream → token array
└────┬────┘
     │  token_array_t
     ▼
┌─────────┐
│ Parser  │  parser.c / ast.c    — token array → AST
└────┬────┘
     │  ast_t (program node)
     ▼
┌───────────┐
│ Generator │  generator.c       — AST → C source file
└────┬──────┘
     │  out.c
     ▼
┌──────────────┐
│ C Compiler   │  gcc (host) or zcc/SDCC (ZXN)
└──────────────┘
```

Invocation:
```bash
rock <input.rkr> [output.exe] [--target=gcc|--target=zxn] [--debug]
```
`rock` is a shell script that runs `rockc` (transpiler binary, Rock → C) then invokes the C compiler. Default output is `<basename>.exe`. `--debug` retains and formats the generated `.c` file.

## Major Components

| Component | Files | Responsibility |
|-----------|-------|----------------|
| **Lexer** | `lexer.c`, `token.c`, `token.h` | Tokenise Rock source into a flat token array |
| **Parser** | `parser.c`, `ast.c`, `ast.h` | Consume tokens, produce AST; resolve includes |
| **Generator** | `generator.c`, `generator.h` | Walk AST, emit C source; manage temporaries and string setup |
| **Name Table** | `name_table.c`, `name_table.h` | Scoped symbol table used by generator for type inference |
| **Runtime Library** | `src/lib/fundefs_internal.c` and headers | Generic array operations, string helpers, memory allocator |
| **Entry Point** | `main.c` | CLI parsing, path resolution, pipeline orchestration |

## Target Platforms

### Host (gcc)
- Generates standard C99-compatible output
- Generated C includes runtime headers by relative name (`#include "alloc.h"`, etc.)
- The `rock` driver compiles with `gcc -Wall -Wno-unused-variable -I "$ROCK_ROOT/src/lib"`
- Full feature support

### ZXN (ZX Spectrum Next / Z80)
- Generates C output compiled by Z88DK's SDCC frontend (`zcc +zxn -clib=sdcc_iy`)
- Generated C includes runtime headers by relative name
- The `rock` driver passes `-I"$ROCK_ROOT/src/lib"`
- Statement splitting: complex expressions written to a `pre_f` buffer then emitted before the statement that uses them
- `zpragma_zxn.inc` configures memory bank layout

See [[targets/host-gcc]] and [[targets/zxn-z80]] for platform detail. The ZXN target has a comprehensive hardware reference starting at [[targets/zxn-hardware]], covering all Next subsystems (ports/registers, memory paging, DMA, palette, ULA, Layer 2, Tilemap, Sprites, Copper, Sound, Keyboard, Interrupts).

Worked ZXN sample programs are catalogued at [[targets/zxn/zxn-sample-programs]]. They show concrete sjasmplus setup sequences for hardware features that Rock may expose through runtime helpers or inline assembly.

## Memory Model

Rock uses a single arena-style allocator (`allocate_compiler_persistent`) — memory is allocated throughout compilation and freed all at once at exit (`kill_compiler_stack`). There is no incremental freeing. String literals in generated C programs are heap-allocated at runtime via `__rock_make_string()`.

## Include System

Rock files may include other Rock files:
```rock
include "path/to/module.rkr"
```
Included files must begin with `module Name;`. The parser splices the included file's token stream at the include site. Circular include protection is tracked per-parse session.

## Type System Summary

| Category | Types |
|----------|-------|
| Integer | `int`, `byte`, `word`, `dword` |
| Text | `string`, `char` |
| Boolean | `boolean` |
| Composite | `record` |
| Enum | `enum` (named integer constants) |
| Union | `union` (one-of-N variants; auto-generated constructors) |
| Array | `Type[]` (dynamic), `Type[N]` (fixed-size) |
| Module | `module` (named singleton struct) |

## Key Architectural Decisions

- **Transpile to C** rather than compile directly — keeps code portable and leverages existing optimising C compilers for both host and Z80.
- **Persistent allocator** — simplifies lifetime management at the cost of peak memory usage.
- **Flat name table** with scope tags — simple to implement, adequate for the language's scope depth.
- **Type-specific array wrappers** synthesised by generator — avoids generics in C whilst maintaining type safety.
- **Statement splitting (pre_f)** for ZXN — emits setup statements before the statement that uses them.

See [[domain-model]] for bounded context map and [[glossary]] for term definitions.
