---
title: "PASTA/80 RTL API Reference"
category: pasta80
tags: [pasta80, rtl, api, reference]
sources: [pasta80]
updated: 2026-04-12
status: current
---

# PASTA/80 RTL API Reference

Complete API surface of the PASTA/80 runtime library. Platform tags indicate availability. Entries marked **[All*]** are compiler built-ins with no RTL source definition.

## Platform Tags

| Tag | Meaning |
|-----|---------|
| **[All]** | Available on every target |
| **[All*]** | Compiler built-in (magic), no RTL source |
| **[CPM]** | CP/M only |
| **[ZX48]** | ZX Spectrum 48K, 128K, and Next |
| **[ZX128]** | ZX Spectrum 128K only |
| **[ZXNext]** | ZX Spectrum Next only |
| **[Agon]** | Agon Light / Console8 |

---

## Constants

| Name | Type | Platform | Notes |
|------|------|----------|-------|
| `True`, `False` | Boolean | [All*] | 1 and 0 |
| `MaxInt` | Integer | [All] | 32767 |
| `MinInt` | Integer | [All] | -32768 |
| `Black`..`White` | Integer | [ZX48] [Agon] | 0-7, mapping differs per platform |
| `ScreenWidth` | Integer | [ZX48] [CPM] [Agon] | 32 (ZX) or 80 (CPM/Agon) |
| `ScreenHeight` | Integer | [ZX48] [CPM] [Agon] | 22 (ZX) or 24 (CPM/Agon) |
| `LineBreak` | String | [ZX48] [CPM] [Agon] | `#13` (ZX) or `#13#10` (CPM/Agon) |

---

## Types

### Primitive Types (All*)

`Integer` (signed 16-bit), `Boolean`, `Char` (8-bit), `Byte` (unsigned 8-bit), `String` (length-prefixed, max 255), `Real` (6-byte Turbo Pascal FP), `Pointer` (16-bit), `File` (untyped), `Text` (text file).

### Internal Types

| Type | Platform | Purpose |
|------|----------|---------|
| `PBlock = ^TBlock` | [All] | Heap free-list pointer |
| `TBlock = record Next: PBlock; Size: Integer end` | [All] | Heap free-list node |
| `FileControlBlock` | [CPM] [ZXNext] [Agon] | Platform-specific file handle (see [[pasta80-rtl-architecture]]) |
| `TextRec` | [CPM] [ZXNext] [Agon] | Text file: FCB + readable/writable flags + 128-byte buffer |
| `FileRec` | [CPM] [ZXNext] [Agon] | Typed file: FCB + component size/count/index + 128-byte buffer |
| `Registers` | [ZXNext] [Agon] | Z80 register set for OS API calls (variant record: byte/word views) |

---

## Procedures

### System

| Signature | Platform | Implementation |
|-----------|----------|----------------|
| `Assert(B: Boolean)` | [All*] | Compiler-generated |
| `Debug` / `Debug(B: Boolean)` | [All*] | Compiler-generated breakpoint |
| `Break`, `Continue`, `Exit` | [All*] | Compiler-generated |
| `Halt` / `Halt(ExitCode: Byte)` | [All*] | Compiler-generated |
| `New(var P)`, `Dispose(P)` | [All*] | Compiler-generated (calls `__getmem`/`__freemem`) |
| `Inc(var V [; N])`, `Dec(var V [; N])` | [All*] | Compiler-generated |
| `Val(S; var Scalar; var E)` | [All*] | Compiler dispatches to `__val_int`/`__val_float`/`__val_enum` |
| `Str(N; var S)` | [All*] | Compiler dispatches to `__strn`/`__strf`/`__stre` |
| `Include(var S; E)`, `Exclude(var S; E)` | [All*] | Compiler-generated |
| `FillChar(var Dest; Len; Data)` | [All*] | → `__fillchar` (ASM: LDIR loop) |
| `Move(var Src, Dst; Count)` | [All] | → `__move` (ASM: LDIR/LDDR with overlap handling) |
| `GetMem(var P; Size)` | [All] | → `__getmem` → `__malloc` (first-fit) |
| `FreeMem(P; Size)` | [All] | → `__freemem` (head insert) |
| `Randomize` | [All] | Inline ASM: seeds from Z80 R register |
| `CheckBreak` | [All] | Ctrl-C / Break+Space check |
| `CheckStack` | [All] | Stack overflow check |

### String Operations

| Signature | Platform | Implementation |
|-----------|----------|----------------|
| `Delete(var S; Start, Count)` | [All] | → `__strdel` (ASM) |
| `Insert(S; var T; Start)` | [All] | → `__strins` (ASM) |

### I/O

| Signature | Platform | Notes |
|-----------|----------|-------|
| `Read(...)`, `ReadLn(...)` | [All*] | Compiler-generated dispatch |
| `Write(...)`, `WriteLn(...)` | [All*] | Compiler-generated dispatch |

### File Operations

| Signature | Platform | Notes |
|-----------|----------|-------|
| `Assign`, `Reset`, `Rewrite`, `Append`, `Close`, `Flush` | [All*] | Compiler maps to `TextAssign`/`FileAssign` etc. |
| `Seek(var F; I)` | [All*] | |
| `Erase(var F)`, `Rename(var F; S)` | [All*] | |
| `BlockRead(var F; var Buf; Count [; var Actual])` | [All*] | |
| `BlockWrite(var F; var Buf; Count [; var Actual])` | [All*] | |

### Screen and Cursor

| Signature | Platform |
|-----------|----------|
| `ClrScr` | [ZX48] [CPM] [Agon] |
| `GotoXY(X, Y)` | [ZX48] [CPM] [Agon] |
| `TextColor(Color)` | [ZX48] [CPM] [Agon] |
| `TextBackground(Color)` | [ZX48] [CPM] [Agon] |
| `CursorOn`, `CursorOff` | [CPM] [Agon] |
| `ClrEol` | [CPM] [Agon] |
| `ClrEos`, `InsLine`, `DelLine` | [CPM] |
| `HighVideo`, `LowVideo`, `NormVideo` | [CPM] [Agon] |
| `Border(Color)` | [ZX48] |

### Graphics

| Signature | Platform |
|-----------|----------|
| `Plot(X, Y)` | [ZX48] [Agon] |
| `Draw(DX, DY)` | [ZX48] [Agon] |
| `Circle(CX, CY, Radius)` | [ZX48] [Agon] |
| `FloodFill(X, Y)` | [ZX48] |
| `Point(X, Y): Boolean` | [ZX48] [Agon] |

### Sound

| Signature | Platform |
|-----------|----------|
| `Sound(Frequency, Duration)` | [ZX48] [Agon] |
| `Beep(Duration: Real; Pitch)` | [ZX48] [Agon] |

### Keyboard and Timing

| Signature | Platform |
|-----------|----------|
| `KeyPressed: Boolean` | [ZX48] [CPM] [Agon] |
| `ReadKey: Char` | [ZX48] [CPM] [Agon] |
| `Delay(Duration)` | [ZX48] [CPM] [Agon] |
| `Frames: Real` | [ZX48] [Agon] |

### Platform-Specific

| Signature | Platform |
|-----------|----------|
| `SelectBank(Bank)` | [ZX128] |
| `SetMemPage(Slot, Page)` | [ZXNext] |
| `SetNextReg(Number, Value)` | [ZXNext] |
| `GetNextReg(Number): Integer` | [ZXNext] |
| `SetCpuSpeed(Value)` | [ZXNext] |
| `GetCpuSpeed: Integer` | [ZXNext] |
| `EsxDos(I; var R): Byte` | [ZXNext] |
| `MosApi(I; var R): Byte` | [Agon] |
| `Bdos(Func [; Param]): Byte` | [CPM] [Agon] |
| `BdosHL(Func [; Param]): Integer` | [CPM] [Agon] |
| `ParamCount: Byte` | [CPM] [Agon] |
| `ParamStr(I): String` | [CPM] [Agon] |

---

## Functions

### Math

`Abs`, `ArcTan`, `Cos`, `Exp`, `Frac`, `Int`, `Ln`, `Log`, `Sin`, `Sqr`, `Sqrt`, `Tan`, `Pi`, `MaxReal`, `MinReal`, `Random(Range)`, `RandomReal`, `Round`, `Trunc` — all [All].

### Ordinal

`Odd`, `Even`, `Ord`, `Pred`, `Succ`, `Chr`, `High`, `Low` — all [All*].

### Bit/Byte

`Hi`, `Lo`, `Swap` — [All], inline ASM.

### Character

`UpCase`, `LoCase` — [All], inline ASM.

### Pointer/Memory

`Addr`, `Ptr`, `SizeOf` — [All*]. `MemAvail`, `MaxAvail` — [All], Pascal (free-list walk).

### String

`Concat` — [All*]. `Copy`, `Length`, `Pos` — [All], ASM.

### File Queries

`Eof`, `Eoln`, `SeekEof`, `SeekEoln`, `FilePos`, `FileSize` — [All*]. `IOResult` — [CPM] [ZXNext] [Agon].

---

## API Surface Summary

| Category | Count | Implementation |
|----------|-------|----------------|
| Compiler built-ins | ~30 | Generated inline by compiler |
| ASM primitives | ~40 | `system.asm`, platform `.asm` files |
| Pascal wrappers | ~20 | `system.pas`, platform `.pas` files |
| Shared file I/O | ~25 | `files.pas` (platform-independent) |
| **Total** | **~115** | |

## See Also

- [[pasta80-rtl-architecture]] — How the layers fit together
- [[pasta80-target-platforms]] — Platform-specific details
