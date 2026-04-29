---
title: Ubiquitous Language — Rock Compiler Glossary
category: overview
tags: [glossary, domain-language]
sources: [samples/im1/main.asm, samples/im2/main.asm, samples/im2hw/main.asm, samples/im2safe/main.asm, samples/sprites/main.asm, samples/sound/main.asm, samples/tilemap/main.asm, samples/layer2-256x192/main.asm, samples/layer2-320x256/main.asm, samples/layer2-640x256/main.asm, samples/copper/main.asm]
updated: 2026-04-28
status: current
---

# Glossary

Canonical glossary for the Rock compiler and language domain. All wiki pages use terms as defined here. Terms are bolded on first use in other pages.

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

### AST (Abstract Syntax Tree)
The tree data structure produced by the parser. Each node is a `node_t` tagged with a `node_tag_t` enum value identifying its kind (e.g. `fundef`, `funcall`, `vardef`, `ifstmt`). The root node has tag `program`.

**Domain:** Parser  
**See also:** [[parser-overview]], [[domain-model]]
---

### AY Register
One of the 14 internal registers on an AY-3-8912 chip. Programs select a register through `$FFFD` with bit 7 clear, then write the value through `$BFFD`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sound]], [[targets/zxn/samples/zxn-sound-sample-summary]]
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

### Builtin Function
A function recognised by name in the generator and translated directly to a runtime call rather than a user-defined `sub`. Examples: `append`, `get`, `set`, `pop`, `insert`, `length`, `concat`, `substring`, `print`, `peek`, `poke`.

**Domain:** Generator  
**See also:** [[generator-overview]]
---

### Clip Window
Layer-specific visible rectangle controlled by sequential Next register writes. Layer 2 uses `$18` for X1/X2/Y1/Y2 and `$1C` bit 1 to reset the write index; wide Layer 2 modes scale X values before storing them.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-layer2]], [[targets/zxn/samples/zxn-layer2-samples]]
---

### Compilation Pipeline
The three sequential phases: Lex → Parse → Generate. Orchestrated by `main.c`.

**Domain:** Architecture  
**See also:** [[overview]], [[concepts/compilation-pipeline]]
---

### Copper
A hardware co-processor on the ZX Spectrum Next that executes a program of WAIT/MOVE/HALT/NOOP instructions in sync with the raster scan. Allows per-scanline register changes without Z80 intervention. Has 2KB of dedicated write-only program memory.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-copper]]
---

### Copper List
Sequence of 16-bit Copper instructions uploaded into Copper memory. A list commonly alternates WAIT and MOVE instructions and ends with HALT so the program can restart cleanly on the next vertical blank.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-copper]], [[targets/zxn/samples/zxn-copper-sample-summary]]
---

### DMA (zxnDMA)
The ZX Spectrum Next's single-channel DMA controller at port `$xx6B`. Transfers memory-to-memory or memory-to-I/O without CPU involvement. Supports fixed-time transfer (prescalar) for sampled audio. Since core 3.1.2, always in zxnDMA mode; legacy Zilog DMA on `$xx0B`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-dma]]
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
**See also:** [[parser-overview]]
---

### Generator
The compiler phase that walks the AST and emits C source. Implemented in `generator.c`. Maintains a name table, a `pre_f` buffer for statement setup code, and a string temporary counter.

**Domain:** Generator  
**See also:** [[generator-overview]]
---

### Hardware IM2
ZX Spectrum Next extension of IM2 that uses a 32-byte aligned vector table and Next interrupt-control registers to route named interrupt sources such as line, ULA, CTC, and UART events.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-interrupts]], [[targets/zxn/samples/zxn-im2hw-sample-summary]]
---

### Host Target
Compilation target. This could be a standard POSIX/Linux/macOS systems, or small system or retro system. Modern systems use C output compiled with `gcc`. The zx next target uses z88dk as its C compiler.

**Domain:** Targets  
**See also:** [[targets/host-gcc]], [[targets/zxn-z80]], [[overview]]
---

### IM1
Z80 interrupt mode 1. On the ZX Spectrum Next, the regular frame interrupt jumps to address `$0038`, so custom IM1 code normally requires paging RAM over the ROM area that contains that address.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-interrupts]], [[targets/zxn/samples/zxn-im1-sample-summary]]
---

### IM2
Z80 interrupt mode 2. The `I` register supplies the high byte of a vector table address, and the data bus supplies the low byte. Robust setups use a 257-byte vector table and a handler address whose high and low bytes match.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-interrupts]], [[targets/zxn/samples/zxn-im2safe-sample-summary]]
---

### Include
A `include "path/to/file.rkr"` directive. The included file's tokens are spliced into the parent token stream at parse time. Included files must begin with `module Name;`.  File paths are relative to the including file.

**Domain:** Parser  
**See also:** [[parser-overview]]
---

### Interrupt Handler
Routine entered by the CPU when a maskable interrupt is accepted. A reusable ZXN handler must preserve any registers it uses, perform source-specific acknowledgement when required, and return with `RETI`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-interrupts]], [[targets/zxn/samples/zxn-interrupt-samples]]
---

### Interrupt Vector Table
Memory table used by IM2-style interrupts to resolve an interrupt event to a handler address. Legacy IM2 uses a 256-byte aligned table; Hardware IM2 uses a 32-byte aligned table with one word per source.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-interrupts]], [[targets/zxn/samples/zxn-interrupt-samples]]
---

### Layer 2
A full-colour framebuffer layer on the ZX Spectrum Next. Supports 256×192 (256 colours), 320×256 (256 colours), or 640×256 (16 colours). Unlike the ULA, every pixel has its own palette index — no colour clash. Lives in contiguous 16K banks starting at bank 9 by default.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-layer2]]
---

### Lexer
The first compiler phase. Converts the Rock source character stream into a flat `token_array_t`. Implemented in `lexer.c`.

**Domain:** Lexer  
**See also:** [[lexer-overview]]
---

### Method
A function defined with `sub Type.method(…)` or `sub Type[].method(…)`. Has an implicit `this` parameter. Mangled in generated C as `TypeName_methodName`.

**Domain:** Parser / Generator  
**See also:** [[syntax/functions-and-methods]]
---

### MMU Slot
One of eight 8K regions in the Z80's 64K address space, managed by the Next MMU paging mode. Registers `$50`–`$57` (MMU0–MMU7) control which 8K bank is mapped into each slot. The only paging mode that gives access to the full 2MB on expanded hardware.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-memory-paging]]
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

### NEX File
Executable file format used by the ZX Spectrum Next. sjasmplus samples produce it with `SAVENEX`; Rock's ZXN driver produces it through `zcc +zxn -subtype=nex`.

**Domain:** Targets  
**See also:** [[targets/zxn-z80]], [[targets/zxn/zxn-sample-programs]]
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

### Parser
The second compiler phase. Converts a `token_array_t` into an AST. Implemented in `parser.c`. Handles includes, operator precedence, and all Rock grammar rules.

**Domain:** Parser  
**See also:** [[parser-overview]]
---

### pre_f Buffer
A secondary output buffer in the generator. Complex sub-expressions (string literals, temporaries) are emitted here first so the surrounding C statement stays syntactically valid. Required for ZXN target; used on host too for consistency.

**Domain:** Generator  
**See also:** [[generator-overview]]
---

### Union (`union`)
A Rock type that holds exactly one of N named variants. Declared with `union Name { type Variant, … }`. Each variant can carry an optional payload (bare variants get an implicit `void` payload). Lowers to a C struct with a `key` enum (which variant is active) and a `value` union (the payloads), plus an auto-generated `Name_Variant(payload)` constructor for each variant.

**Domain:** Parser / Generator  
**See also:** [[syntax/modules-and-records]]
---

### Record
A composite value type declared with `record Name { field: type, … }`. Maps to a C struct. Instantiated with `record { field := value, … }`.

**Domain:** Parser / Generator  
**See also:** [[syntax/modules-and-records]]
---

### Region
A runtime allocation lifetime bucket. Values such as strings, arrays, records,
and unions may point at memory in a region, but the value descriptors do not
own that memory individually. Reclamation happens by resetting or destroying
the whole region.

**Domain:** Runtime library / memory model  
**See also:** [[decisions/ADR-0002-string-view-memory-model]]
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

### sjasmplus
Assembler used by the ZXN sample programs. It supports `DEVICE ZXSPECTRUMNEXT`, `NEXTREG`, bank/page directives, `INCBIN`, and `SAVENEX` output generation.

**Domain:** Targets  
**See also:** [[targets/zxn/zxn-sample-programs]]
---

### Sprite
A 16×16 pixel hardware-rendered object on the ZX Spectrum Next. Pattern data lives in 16KB of dedicated FPGA memory. Up to 128 simultaneous sprites. Can be 8-bit (256 colour) or 4-bit (16 colour). Supports scaling, mirroring, rotation, and anchor/relative grouping.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sprites]]
---

### Sprite Pattern
Pixel data for one hardware sprite image. In 8-bit mode each 16x16 pattern uses 256 bytes; in 4-bit mode each 16x16 pattern uses 128 bytes. Patterns are uploaded to dedicated FPGA sprite memory through port `$xx5B`, often with DMA.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sprites]], [[targets/zxn/samples/zxn-sprite-sample-summary]]
---

### Startup CRT
Z88DK runtime startup profile selected with `-startup=n`. It controls runtime initialisation before `main()`, including stdout/stderr driver setup, control-code support, and minimal no-stdio builds.

**Domain:** Targets  
**See also:** [[targets/zxn-z80]], [[targets/zxn/zx-z88dk-startupcrt-summary]]
---

### Statement Splitting
The technique of separating complex expression setup from the statement that uses it, writing setup into `pre_f` and the main expression into `f`. Required to satisfy C grammar constraints on ZXN/SDCC.

**Domain:** Generator  
**See also:** [[generator-overview]], [[targets/zxn-z80]]
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

### Tile Definition
Pixel data for one 8x8 tile used by the Tilemap layer. In 4-bit tile mode each definition uses 32 bytes, and definitions are stored in bank 5 at the address selected by `$6F`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-tilemap]], [[targets/zxn/samples/zxn-tilemap-sample-summary]]
---

### Tilemap
A block-based display layer on the ZX Spectrum Next using 8×8 pixel tile definitions. Two resolutions: 40×32 or 80×32 tiles (full screen including border). 4 bits per pixel per tile, with per-tile palette offset, mirroring, and rotation attributes. Tile data lives in 16K bank 5.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-tilemap]]
---

### Tilemap Entry
The per-cell value in tilemap memory. In one-byte mode it is only the tile index; in two-byte mode it pairs an attribute byte with an index byte.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-tilemap]], [[targets/zxn/samples/zxn-tilemap-sample-summary]]
---

### Token
The unit of output from the lexer. A `token_t` struct with `type` (enum), `lexeme` (string view), `line`, `col`, and for embed tokens, `embed_body` and `embed_lang`.

**Domain:** Lexer  
**See also:** [[lexer-overview]]
---

### Transpile
The process of converting Rock source to C source. Distinguished from compilation (C→machine code) which is handled by gcc or zcc.

**Domain:** Architecture  
**See also:** [[overview]]
---

### Turbo Sound Next
ZX Spectrum Next audio extension that provides three AY-3-8912 chips and 9 total tone channels. Programs select the active chip through `$FFFD` and enable Turbo Sound behaviour with Peripheral 3 `$08`.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sound]], [[targets/zxn/samples/zxn-sound-sample-summary]]
---

### ULA (Uncommitted Logic Array)
The original ZX Spectrum display chip. Provides a 256×192 pixel display using 1 bit per pixel in a non-linear memory layout at `$4000–$57FF`, with 8×8 character-block colour attributes at `$5800–$5AFF`. Always reads from 16K bank 5 unless redirected to bank 7 (shadow screen).

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-ula]]
---

### Unified Relative Sprite
Sprite grouping mode where relative sprites inherit the anchor's transform so a multi-pattern object moves, rotates, mirrors, and scales as one unit.

**Domain:** ZXN Hardware  
**See also:** [[targets/zxn/zxn-sprites]], [[targets/zxn/samples/zxn-sprite-sample-summary]]
---

### ZXN Sample Program
A small sjasmplus assembly program used as an executable reference for one ZX Spectrum Next hardware feature. Sample pages link each program to the subsystem pages it exercises.

**Domain:** Targets  
**See also:** [[targets/zxn/zxn-sample-programs]]
---

### ZXN Target
Compilation target for the ZX Spectrum Next (Z80 architecture). C output compiled with `zcc +zxn -clib=sdcc_iy` (Z88DK with SDCC backend). Requires relative includes and statement splitting.

**Domain:** Targets  
**See also:** [[targets/zxn-z80]], [[overview]]
---

### z80asm
The relocatable macro assembler, linker, and librarian that forms the backend of Z88DK. Invoked by `zcc` internally; also available directly as `z88dk-z80asm`. Supports Z80N opcodes (`-mz80n`) for ZX Spectrum Next targets, `CU.*` Copper directives, and `DMA.*` DMA directives.

**Domain:** Targets  
**See also:** [[targets/zxn/tools/z80asm-reference]], [[targets/zxn-z80]]
---

### ASMPC
Assembly program counter — the address of the current instruction at the point z80asm is assembling it. Used in expressions to compute relative offsets or data sizes. Resolved at link time.

**Domain:** Targets  
**See also:** [[targets/zxn/tools/z80asm-reference]]
---

### Calling Convention
The protocol by which the caller and callee agree on how parameters are passed (registers vs. stack), who cleans the stack, and where return values are placed. Z88DK with SDCC (`-clib=sdcc_iy`) uses a register-based convention for small parameters; the stack is used for overflow. `@embed asm` blocks run inside an SDCC-managed frame — IY must not be modified.

**Domain:** Targets  
**See also:** [[targets/zxn/tools/z88dk-inline-asm]], [[targets/zxn-z80]]
