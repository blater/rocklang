---
title: Ubiquitous Language — Rock Compiler Glossary
category: overview
tags: [glossary, domain-language]
sources: []
updated: 2026-04-10
status: current
---

# Ubiquitous Language

Canonical glossary for the Rock compiler and language domain. All wiki pages use terms as defined here. Terms are bolded on first use in other pages.

---

### AST (Abstract Syntax Tree)
The tree data structure produced by the parser. Each node is a `node_t` tagged with a `node_tag_t` enum value identifying its kind (e.g. `fundef`, `funcall`, `vardef`, `ifstmt`). The root node has tag `program`.

**Domain:** Parser  
**See also:** [[parser/parser-overview]], [[domain-model]]

---

### Arena Allocator
Rock's memory strategy: `allocate_compiler_persistent()` allocates from a growing stack; `kill_compiler_stack()` frees everything at once. No incremental freeing occurs during compilation.

**Domain:** Runtime / compiler infrastructure  
**See also:** [[concepts/array-internals]]

---

### Array (dynamic)
A heap-allocated, growable sequence. Represented at runtime as `__internal_dynamic_array_t` with fields `data`, `length`, `capacity`, `elem_size`, `max_capacity`. When `max_capacity == 0` the array is dynamic (doubles on overflow); when set, it is fixed-size.

**Domain:** Runtime library  
**See also:** [[concepts/array-internals]]

---

### Array (fixed-size)
A `Type[N]` declaration in Rock. Uses the same `__internal_dynamic_array_t` struct as a dynamic array but with `max_capacity` set to `N`. Append/insert beyond capacity halts the program.

**Domain:** Runtime library  
**See also:** [[concepts/array-internals]]

---

### Builtin Function
A function recognised by name in the generator and translated directly to a runtime call rather than a user-defined `sub`. Examples: `append`, `get`, `set`, `pop`, `insert`, `length`, `concat`, `substring`, `print`, `peek`, `poke`.

**Domain:** Generator  
**See also:** [[generator/generator-overview]]

---

### Compilation Pipeline
The three sequential phases: Lex → Parse → Generate. Orchestrated by `main.c`.

**Domain:** Architecture  
**See also:** [[overview]], [[concepts/compilation-pipeline]]

---

### Embed Block
An `@embed lang … @end lang` section in Rock source containing raw C or Z80 assembly. The lexer captures the body verbatim in the token; the generator emits it unchanged.

**Domain:** Lexer / Generator  
**See also:** [[syntax/embed]]

---

### Enum
A Rock type declared with `enum Name { Item1, Item2, … }`. Maps to a C `typedef enum`.

**Domain:** Parser / Generator  
**See also:** [[syntax/modules-and-records]]

---

### fundef
The AST node type for function and method definitions. Fields include name, return type, parameter list, method flags (`is_method`, `is_array_method`), and body.

**Domain:** Parser  
**See also:** [[parser/parser-overview]]

---

### Generator
The compiler phase that walks the AST and emits C source. Implemented in `generator.c`. Maintains a name table, a `pre_f` buffer for statement setup code, and a string temporary counter.

**Domain:** Generator  
**See also:** [[generator/generator-overview]]

---

### Host Target
Compilation target for standard POSIX/Linux/macOS systems. C output compiled with `gcc`. Includes use absolute paths.

**Domain:** Targets  
**See also:** [[targets/host-gcc]], [[overview]]

---

### Include
A `include "path/to/file.rkr"` directive. The included file's tokens are spliced into the parent token stream at parse time. Included files must begin with `module Name;`.

**Domain:** Parser  
**See also:** [[parser/parser-overview]]

---

### Lexer
The first compiler phase. Converts the Rock source character stream into a flat `token_array_t`. Implemented in `lexer.c`.

**Domain:** Lexer  
**See also:** [[lexer/lexer-overview]]

---

### Method
A function defined with `sub Type.method(…)` or `sub Type[].method(…)`. Has an implicit `this` parameter. Mangled in generated C as `TypeName_methodName`.

**Domain:** Parser / Generator  
**See also:** [[syntax/functions-and-methods]]

---

### Module
A named singleton struct type declared with `module Name;`. Field declarations follow in the same file. The generator synthesises a `TypeName_new()` constructor.

**Domain:** Parser / Generator  
**See also:** [[syntax/modules-and-records]]

---

### Name Table
The compiler's symbol table. A flat array of entries with scope depths. Implemented in `name_table.c`. Used by the generator to look up types and resolve method call receivers.

**Domain:** Generator / Compiler infrastructure  
**See also:** [[concepts/name-table]]

---

### Parser
The second compiler phase. Converts a `token_array_t` into an AST. Implemented in `parser.c`. Handles includes, operator precedence, and all Rock grammar rules.

**Domain:** Parser  
**See also:** [[parser/parser-overview]]

---

### pre_f Buffer
A secondary output buffer in the generator. Complex sub-expressions (string literals, temporaries) are emitted here first so the surrounding C statement stays syntactically valid. Required for ZXN target; used on host too for consistency.

**Domain:** Generator  
**See also:** [[generator/generator-overview]]

---

### Product Type (pro)
A Rock type declared with `pro Name { Constructor: type, … }`. A tagged-union-like construct. Maps to a C struct with a tag field.

**Domain:** Parser / Generator  
**See also:** [[syntax/modules-and-records]]

---

### Record
A composite value type declared with `record Name { field: type, … }`. Maps to a C struct. Instantiated with `record { field := value, … }`.

**Domain:** Parser / Generator  
**See also:** [[syntax/modules-and-records]]

---

### Rock
The language being transpiled. Statically typed, C-targeting, with syntax inspired by BASIC-era structured languages. File extension `.rkr`.

**Domain:** Language  
**See also:** [[overview]]

---

### Scope
A lexical nesting level in the name table. Scope 0 = top-level / global. Functions and loops create new scopes. On scope exit, entries deeper than the current depth are truncated from the name table.

**Domain:** Compiler infrastructure  
**See also:** [[concepts/name-table]]

---

### Statement Splitting
The technique of separating complex expression setup from the statement that uses it, writing setup into `pre_f` and the main expression into `f`. Required to satisfy C grammar constraints on ZXN/SDCC.

**Domain:** Generator  
**See also:** [[generator/generator-overview]], [[targets/zxn-z80]]

---

### String Temporary
A generated variable `__strtmp_N` used to hold an intermediate string value during expression evaluation. Counter managed by the generator per-function.

**Domain:** Generator  
**See also:** [[concepts/string-representation]]

---

### sub
The Rock keyword for defining a function or method (`sub name(params): rettype { body }`).

**Domain:** Language / Parser  
**See also:** [[syntax/functions-and-methods]]

---

### Token
The unit of output from the lexer. A `token_t` struct with `type` (enum), `lexeme` (string view), `line`, `col`, and for embed tokens, `embed_body` and `embed_lang`.

**Domain:** Lexer  
**See also:** [[lexer/lexer-overview]]

---

### Transpile
The process of converting Rock source to C source. Distinguished from compilation (C→machine code) which is handled by gcc or zcc.

**Domain:** Architecture  
**See also:** [[overview]]

---

### AY-3-8912
Programmable sound chip. The ZX Spectrum Next includes three of these chips (Turbo Sound Next). Each has 3 tone channels, 1 noise generator, and an envelope generator. Programmed via I/O ports `$FFFD` (chip/register select) and `$BFFD` (register write).

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sound]]

---

### Bank (memory bank)
A fixed-size block of physical RAM that can be mapped into a CPU address slot. The Next supports **8K banks** (for the MMU paging mode) and **16K banks** (for legacy 128K/+3 compatibility). Bank 0 starts at absolute address `$40000`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-memory-paging]]

---

### Copper
A hardware co-processor on the ZX Spectrum Next that executes a program of WAIT/MOVE/HALT/NOOP instructions in sync with the raster scan. Allows per-scanline register changes without Z80 intervention. Has 2KB of dedicated write-only program memory.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-copper]]

---

### DMA (zxnDMA)
The ZX Spectrum Next's single-channel DMA controller at port `$xx6B`. Transfers memory-to-memory or memory-to-I/O without CPU involvement. Supports fixed-time transfer (prescalar) for sampled audio. Since core 3.1.2, always in zxnDMA mode; legacy Zilog DMA on `$xx0B`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-dma]]

---

### Layer 2
A full-colour framebuffer layer on the ZX Spectrum Next. Supports 256×192 (256 colours), 320×256 (256 colours), or 640×256 (16 colours). Unlike the ULA, every pixel has its own palette index — no colour clash. Lives in contiguous 16K banks starting at bank 9 by default.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-layer2]]

---

### MMU Slot
One of eight 8K regions in the Z80's 64K address space, managed by the Next MMU paging mode. Registers `$50`–`$57` (MMU0–MMU7) control which 8K bank is mapped into each slot. The only paging mode that gives access to the full 2MB on expanded hardware.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-memory-paging]]

---

### Next Register
A hardware control register on the ZX Spectrum Next, accessed via ports `$243B` (select) and `$253B` (read/write), or the Z80N `NEXTREG` instruction. There are ~130 registers (`$00`–`$FF`) controlling all hardware subsystems.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-ports-registers]]

---

### NEXTREG
A Z80N extended instruction that writes a value directly to a Next register. Syntax: `NEXTREG reg, value` (24 T-states) or `NEXTREG reg, A` (20 T-states). Preferred over the `$243B`/`$253B` port method (52–58 T-states).

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-ports-registers]]

---

### Palette (ZXN)
A 256-entry lookup table mapping colour indices to RRRGGGBB (8-bit) or RRRGGGBBB (9-bit) RGB values. Each graphics layer (ULA, Layer 2, Sprites, Tilemap) has two palettes. Edited via Next registers `$40`–`$44`. The active palette is selected per-layer via `$43`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-palette]]

---

### Palette Offset
A 4-bit value attached to each sprite or tilemap tile that shifts its colour indices by `offset × 16`. Allows each sprite/tile to select one of 16 sub-palettes of 16 colours from the 256-entry palette.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sprites]], [[targets/zxn/zxn-tilemap]]

---

### Sprite (hardware)
A 16×16 pixel hardware-rendered object on the ZX Spectrum Next. Pattern data lives in 16KB of dedicated FPGA memory. Up to 128 simultaneous sprites. Can be 8-bit (256 colour) or 4-bit (16 colour). Supports scaling, mirroring, rotation, and anchor/relative grouping.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sprites]]

---

### Tilemap
A block-based display layer on the ZX Spectrum Next using 8×8 pixel tile definitions. Two resolutions: 40×32 or 80×32 tiles (full screen including border). 4 bits per pixel per tile, with per-tile palette offset, mirroring, and rotation attributes. Tile data lives in 16K bank 5.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-tilemap]]

---

### ULA (Uncommitted Logic Array)
The original ZX Spectrum display chip. Provides a 256×192 pixel display using 1 bit per pixel in a non-linear memory layout at `$4000–$57FF`, with 8×8 character-block colour attributes at `$5800–$5AFF`. Always reads from 16K bank 5 unless redirected to bank 7 (shadow screen).

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-ula]]

---

### ZXN Target
Compilation target for the ZX Spectrum Next (Z80 architecture). C output compiled with `zcc +zxn -clib=sdcc_iy` (Z88DK with SDCC backend). Requires relative includes and statement splitting.

**Domain:** Targets  
**See also:** [[targets/zxn-z80]], [[overview]]
