---
title: Domain Model — Bounded Contexts
category: overview
tags: [domain-model, architecture, bounded-contexts]
sources: []
updated: 2026-04-09
status: current
---

# Domain Model

## Bounded Context Map

```mermaid
graph TD
    subgraph Compiler["Compiler Pipeline"]
        LEX[Lexer\nlexer.c / token.c]
        PAR[Parser\nparser.c / ast.c]
        GEN[Generator\ngenerator.c]
    end

    subgraph Infrastructure["Compiler Infrastructure"]
        NT[Name Table\nname_table.c]
        ALLOC[Arena Allocator\nalloc.h]
        SV[String View\nstringview.h]
    end

    subgraph Runtime["Runtime Library"]
        FUND[Array & String Ops\nfundefs_internal.c]
        ASM[Z80 Interop\nasm_interop.h]
    end

    subgraph Targets["Output Targets"]
        HOST[Host / gcc]
        ZXN[ZXN / Z88DK+SDCC]
    end

    subgraph ZXNHardware["ZXN Hardware"]
        MEM[Memory & Paging\nMMU $50-$57]
        GFX[Graphics Pipeline\nULA / Layer2 / Tilemap / Sprites]
        DMA[DMA\nport $xx6B]
        COP[Copper\n$60-$63]
        SND[Sound\nAY-3-8912 x3]
        INT[Interrupts\nIM1/IM2/HW-IM2]
    end

    SRC[Rock Source .rkr] --> LEX
    LEX --> PAR
    PAR --> GEN
    GEN --> NT
    GEN --> HOST
    GEN --> ZXN
    NT --> ALLOC
    LEX --> SV
    PAR --> ALLOC
    HOST --> FUND
    ZXN --> FUND
    ZXN --> ASM
    ZXN --> ZXNHardware
    MEM --> GFX
    DMA --> GFX
    COP --> GFX
    INT --> DMA
```

---

## Bounded Contexts

### Lexer Context
**Owns:** Token production, lexeme extraction, line/column tracking, embed block capture.  
**Invariant:** Each call to `step_lexer()` consumes characters and returns exactly one token. The lexer never looks back.  
**Exports:** `token_array_t` — a flat, heap-allocated array of `token_t`.  
**Dependencies:** `string_view` (zero-copy lexeme representation), arena allocator.

### Parser Context
**Owns:** Grammar rules, AST construction, include resolution, operator precedence.  
**Invariant:** Consumes tokens strictly left-to-right. Includes are spliced at parse time; after splicing, the token stream appears flat.  
**Exports:** `ast_t` — the root `program` node containing all top-level definitions.  
**Dependencies:** Lexer (token stream), arena allocator.  
**Notable constraint:** No semantic analysis. Type checking is deferred to the generator.

### Generator Context
**Owns:** C code emission, type inference, string temporary management, type-specific array wrapper synthesis, module initialisation deferral.  
**Invariant:** Single-pass AST traversal. Name table reflects current scope. `pre_f` buffer is flushed before each top-level statement.  
**Exports:** A `.c` source file ready for gcc or zcc.  
**Dependencies:** Name table (symbol lookup), AST (parser output), target flag (HOST vs ZXN).

### Name Table Context
**Owns:** Symbol registration, scope lifecycle, name resolution.  
**Invariant:** Entries are never individually deleted. Scope exit truncates the entry list to the depth boundary.  
**Exports:** `get_ref()` returning the AST node registered for a name; `push_nt()` for registration.  
**Dependencies:** Arena allocator.

### Runtime Library Context
**Owns:** Dynamic array implementation, string construction helpers, command-line argument access, Z80 assembly interop stubs.  
**Invariant:** All allocations via arena — no individual frees.  
**Exports:** `__internal_*` functions (generic array ops), type-specific wrappers (`int_push_array`, etc.), `__rock_make_string`, `fill_cmd_args`.  
**Dependencies:** Arena allocator (`alloc.h`).

### ZXN Hardware Context
**Owns:** All ZX Spectrum Next hardware subsystem definitions — ports, registers, graphics layers, DMA, sound, keyboard, interrupts.  
**Invariant:** Hardware state is write-only from the Z80 perspective for most registers; reads are available for status flags and read-back registers.  
**Exports:** Register addresses and bit-field specifications consumed when implementing Rock runtime functions for ZXN.  
**Key subsystems:** Memory/MMU paging, ULA, Layer 2, Tilemap, Sprites, Palette, Copper, DMA, AY sound, Keyboard, Interrupts.  
**Reference:** [[targets/zxn-hardware]] hub → individual subsystem pages in `pages/targets/zxn/`.

### Target Context
**Owns:** Platform-specific include strategy, compiler flags, memory layout (`zpragma_zxn.inc`).  
**Invariant:** Target is fixed at transpile time (set by `--target=zxn` flag).  
**Exports:** Target enum (`TARGET_HOST`, `TARGET_ZXN`) consumed by generator.

---

## Key Integration Points

| From | To | Integration |
|------|----|-------------|
| Lexer → Parser | `token_array_t` passed by value | Flat token array; cursor managed by parser |
| Parser → Generator | `ast_t` (root node) | Single root node with nested child arrays |
| Generator → Name Table | `push_nt()` / `get_ref()` | Called during AST traversal for every definition and lookup |
| Generator → Target | `generator.target` flag | Determines include path style, statement splitting, Z80 pragmas |
| Runtime → Generator | Header contracts | Generator emits calls matching runtime function signatures |

---

## Notable Constraints Across Contexts

- **No separate semantic analysis phase.** Type checking, method resolution, and type inference all happen inside the generator during code emission. Errors surface as C compiler errors or runtime crashes, not as Rock-level error messages.
- **Include resolution at parse time.** This means included files are fully parsed before the parent file's parse continues. Circular includes are detected but only at file-name level.
- **Arena allocator spans all contexts.** Everything allocated during a compile shares the same lifetime. This simplifies lifetime management but means no memory is reclaimed until `kill_compiler_stack()` at exit.

See [[overview]] for the pipeline diagram and [[ubiquitous-language]] for term definitions.
