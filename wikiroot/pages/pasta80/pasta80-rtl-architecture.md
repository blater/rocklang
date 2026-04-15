---
title: "PASTA/80 RTL Architecture"
category: pasta80
tags: [pasta80, rtl, architecture, hal, assembly, pascal]
sources: [pasta80]
updated: 2026-04-12
status: current
---

# PASTA/80 RTL Architecture

The PASTA/80 runtime library uses a **4-layer architecture** with a **Hardware Abstraction Layer (HAL)** pattern that allows a single shared file-I/O implementation to work across all platforms that support files.

## Layer Diagram

```
┌─────────────────────────────────────────────────────────┐
│  Layer 3: Shared Abstractions                           │
│  files.pas — TextRec, FileRec, text/typed file I/O      │
│  (Written once. Works on any platform that provides     │
│   the Block* HAL below.)                                │
├─────────────────────────────────────────────────────────┤
│  Layer 2: Platform HAL                                  │
│  ┌──────────┬──────────┬──────────┬──────────┐          │
│  │ cpm.pas  │ zxrom.pas│esxdos.pas│ agon.pas │          │
│  │ cpm.asm  │ zxrom.asm│esxdos.asm│ agon.asm │          │
│  │          │          │tbblue.pas│agonhead  │          │
│  │          │          │tbblue.asm│agonover  │          │
│  └──────────┴──────────┴──────────┴──────────┘          │
│  Each defines: FileControlBlock + Block* procedures     │
│  + platform-specific screen/keyboard/sound/graphics     │
├─────────────────────────────────────────────────────────┤
│  Layer 1: Core Pascal RTL                               │
│  system.pas — types, heap (PBlock/TBlock), math funcs,  │
│  string funcs, constants, Move, FillChar, Random,       │
│  assertion support, CheckBreak, CheckStack              │
├─────────────────────────────────────────────────────────┤
│  Layer 0: Assembly Primitives                           │
│  system.asm — int16 arithmetic, mul/div, string ops,    │
│  heap allocator (malloc/freemem), set operations,       │
│  formatted I/O, Val/Str conversions                     │
│  math48.asm — 6-byte floating-point (Hejlsberg)         │
└─────────────────────────────────────────────────────────┘
```

## Target Entry Points

Each target has a top-level `.pas` file that `{$i}`-includes the appropriate layers:

| Target | Entry File | Includes |
|--------|-----------|----------|
| CP/M | `cpm.pas` | system.pas, `{$l cpm.asm}`, Block* impl, `{$i files.pas}` |
| ZX 48K | `zx.pas` | system.pas, zxrom.pas |
| ZX 128K | `zx128.pas` | system.pas, zxrom.pas, derby.pas |
| Next | `next.pas` | system.pas, zxrom.pas, esxdos.pas, files.pas, tbblue.pas |
| Agon | `agon.pas` | `{$l agonhead.asm}`, system.pas, `{$l agon.asm}`, Block* impl, `{$i files.pas}`, graphics |

Key observation: **ZX 48K and ZX 128K do not include `files.pas`** because they lack file system access. The Next and Agon targets each provide their own `FileControlBlock` and `Block*` implementation, then include the shared `files.pas`.

## The HAL Pattern (FileControlBlock + Block*)

The most important architectural pattern in the RTL is the **Block* HAL**. Each platform that supports files defines:

### 1. A `FileControlBlock` record (different per platform)

```
CP/M:       36 bytes — DR, FN[8], TN[3], CP/M fields, RL/RH (24-bit record), SL/SH (size)
Next:       36 bytes — Handle (esxDOS), FileName[32], RL/RH (record pos)
Agon:       69 bytes — Handle (MOS), FileName[64], RL (record pos), SL (size)
```

### 2. A set of `Block*` procedures (same signatures everywhere)

| Procedure | Purpose |
|-----------|---------|
| `BlockAssign(var F; S: String)` | Associate filename with FCB |
| `BlockReset(var F)` | Open for reading |
| `BlockRewrite(var F)` | Create/truncate for writing |
| `BlockClose(var F)` | Close file |
| `BlockSeek(var F; I: Integer)` | Seek to record position |
| `BlockBlockRead(var F; var Buf; Count; var Actual)` | Read 128-byte records |
| `BlockBlockWrite(var F; var Buf; Count; var Actual)` | Write 128-byte records |
| `BlockErase(var F)` | Delete file |
| `BlockRename(var F; S: String)` | Rename file |
| `BlockFilePos(var F): Integer` | Current record position |
| `BlockFileSize(var F): Integer` | File size in records |
| `BlockEof(var F): Boolean` | End-of-file check |

### 3. `files.pas` consumes these Block* procedures

`files.pas` defines `TextRec` and `FileRec` record types, each containing a `FileControlBlock` as the first field. It implements buffered text I/O (`TextReadChar`, `TextWriteChar`, `TextReadStr`, `TextWriteStr`, etc.) and typed file I/O (`FileRead`, `FileWrite`, `FileSeek`, `FileClose`) entirely in terms of the Block* calls.

This means **files.pas is written once** and works on CP/M, Next, and Agon without modification.

## Mixed Pascal/Assembly Strategy

The RTL uses a deliberate split between Pascal and assembly:

### In Assembly (performance-critical, low-level)
- **Integer arithmetic**: 16-bit multiply, divide, shift, comparison, negation
- **String operations**: compare, concatenate, pos, copy, insert, delete
- **Heap allocator**: `__malloc` (first-fit free-list walk), `__freemem` (insert into free list)
- **Set operations**: membership test, union, intersection, difference, equality
- **Floating-point math**: all 6-byte FP operations via math48.asm (Anders Hejlsberg's original code)
- **I/O formatting**: integer-to-string, string-to-integer, formatted output with field widths
- **Platform init/shutdown**: `__init` (set SP, call main), `__done` (RST 0 / halt)

### In Pascal (logic-heavy, maintainability matters)
- **Heap management wrappers**: `MemAvail`, `MaxAvail`, `InitHeap` (walk free list, compute totals)
- **File I/O buffering**: all of `files.pas` — sector buffering, typed file seek with Real arithmetic
- **Command-line parsing**: `ParamCount`, `ParamStr` (string scanning)
- **Sound/music**: frequency calculation with floating-point math
- **High-level graphics**: `Circle` (Bresenham midpoint), `Draw` (delegates to ASM)
- **Error handling**: `BDosThrow`, `BDosCatch`, `IOResult`

### The `external` and `inline` keywords

Pascal declarations are linked to assembly via two mechanisms:

```pascal
(* Link to assembly label *)
function Length(S: String): Integer; external '__length';

(* Embed raw Z80 opcodes directly *)
function Hi(I: Integer): Byte; register; inline
(
  $6c /       (* ld   l,h     *)
  $26 / $00 / (* ld   h,0     *)
  $c9         (* ret          *)
);
```

The `inline` mechanism is used for very small functions (3-10 bytes) where the call overhead would exceed the function body. The `register` keyword indicates the register calling convention (arguments in HL, DE, BC rather than on the stack).

## Conditional Compilation

`system.pas` uses `{$ifdef}` to handle minor platform differences at the core level:

```pascal
{$ifdef sys_agon}
const
  Blue = 4; Red = 1; Green = 2;  (* Agon colour mapping *)
{$else}
const
  Blue = 1; Red = 2; Green = 4;  (* ZX Spectrum colour mapping *)
{$endif}
```

The compiler pre-defines symbols: `PASTA`, `CPU_Z80`, `CPU_Z80N`, `CPU_EZ80`, `SYS_CPM`, `SYS_ZX48`, `SYS_ZX128`, `SYS_ZXNEXT`, `SYS_AGON`.

## Heap Design

The heap uses a **singly-linked free list** with 4-byte block headers:

```
┌──────────┬──────────┐
│ Next (2) │ Size (2) │  ← PBlock/TBlock
├──────────┴──────────┤
│ ... usable memory ...│
└─────────────────────┘
```

- **Allocation** (`__malloc`): first-fit walk. Exact match removes block from list; larger blocks are split if >=4 bytes remain.
- **Deallocation** (`__freemem`): inserts freed block at head of list (O(1), no coalescing).
- **Initialisation** (`InitHeap`): creates a single free block from end-of-program to stack base (`$DFFF` = 57343).
- **Introspection**: `MemAvail` sums all free blocks; `MaxAvail` returns the largest.

> **TODO:** The lack of free-block coalescing means heap fragmentation grows over time. This is a known limitation.

## Error Handling Pattern

File operations use a global `LastError: Byte` variable:

1. Every `Block*` procedure checks `if LastError <> 0 then Exit` at entry — cascading errors skip subsequent operations
2. `BDosCatch` wraps OS calls and sets `LastError` on failure
3. In `{$i+}` mode (IO checking on), the compiler inserts calls to `BDosThrow` after file operations, which halts on error
4. In `{$i-}` mode, the user checks `IOResult` manually (which reads and clears `LastError`)

This pattern is replicated identically on CP/M (BDOS calls), Next (esxDOS calls), and Agon (MOS API calls).

## See Also

- [[pasta80/pasta80-overview]] — Project overview
- [[pasta80/pasta80-rtl-api]] — Full API reference
- [[pasta80/pasta80-target-platforms]] — Platform-specific implementation details
- [[pasta80/pasta80-lessons-for-rock]] — Lessons for Rock's RTL
