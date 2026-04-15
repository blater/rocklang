---
title: "PASTA/80 Target Platforms"
category: pasta80
tags: [pasta80, targets, cpm, zx-spectrum, zx-next, agon, z80]
sources: [pasta80]
updated: 2026-04-12
status: current
---

# PASTA/80 Target Platforms

Each target platform has its own RTL entry point, platform-specific assembly, and (where applicable) file I/O implementation. This page details how each target is composed and what it provides.

## CP/M (`cpm.pas` + `cpm.asm`)

**The reference platform** — most complete feature set, most straightforward implementation.

### Composition

```
cpm.pas
  {$i system.pas}     → Layer 1 core + Layer 0 (system.asm)
  {$l cpm.asm}         → CP/M-specific ASM
  ... VT52 terminal, BDOS, command-line parsing ...
  {$i files.pas}       → Shared file abstraction
```

### Key Implementation Details

| Aspect | Implementation |
|--------|---------------|
| **Console output** | BDOS call 2 (`__putc`: `ld c,2; call 5`) |
| **Line input** | BDOS call 10 (buffered), or custom `__getline` with `__readkey` |
| **Key detection** | BDOS call 11 (`KeyPressed`), BDOS call 6/255 (`ReadKey`) |
| **Screen control** | VT52 escape sequences via `ConOut` (ESC+Y for cursor, ESC+J for clear, etc.) |
| **Colour** | VT52 colour escapes: ESC+T+digit (foreground), ESC+S+digit (background) |
| **File I/O** | CP/M FCB-based: BDOS 15 (open), 16 (close), 22 (create), 33 (random read), 34 (random write), 35 (file size) |
| **File naming** | 8.3 format, drive letter prefix (`A:FILE.TXT`), space-padded fields |
| **Command line** | Parsed from CP/M's `$80` command tail (length-prefixed string) |
| **Timing** | BDOS call 141 (CP/M 3 delay, 20ms ticks) |
| **Init/Shutdown** | `__init`: set SP from `($0006)`, call main. `__done`: RST 0 (warm boot) |

### FileControlBlock (CP/M)

```pascal
FileControlBlock = record
  DR: Byte;                    (* Drive number, 0=default *)
  FN: array[0..7] of Char;    (* Filename, space-padded *)
  TN: array[0..2] of Char;    (* Extension, space-padded *)
  EX, S1, S2, RC: Byte;       (* CP/M internal *)
  AL: array[0..15] of Byte;   (* CP/M internal *)
  CR: Byte;                    (* CP/M internal *)
  RL: Integer; RH: Byte;      (* 24-bit random record number *)
  SL: Integer; SH: Byte;      (* Record count (not CP/M standard) *)
end;
```

All file operations work in 128-byte records (the CP/M sector size), using BDOS random read/write calls.

---

## ZX Spectrum 48K (`zx.pas` + `zxrom.pas` + `zxrom.asm`)

**Minimal target** — screen and keyboard only, no files, no overlays.

### Composition

```
zx.pas
  {$i system.pas}     → Layer 1 core + Layer 0
  {$i zxrom.pas}       → ZX screen/keyboard/sound/graphics
                         {$l zxrom.asm}
```

### Key Implementation Details

| Aspect | Implementation |
|--------|---------------|
| **Console output** | ROM RST $10 (print character, handles control codes) |
| **Screen control** | ROM routines: `zx_clrscr`, `zx_gotoxy` (set PRINT AT position) |
| **Colour** | ZX attribute system: `zx_color` (INK), `zx_background` (PAPER) |
| **Key detection** | `zx_testkey` (port reads), `zx_readkey` (wait + decode) |
| **Graphics** | `zx_plot` (set pixel), `zx_point` (read pixel), `zx_draw` (Bresenham in ASM), `zx_fill` (flood fill in ASM), `Circle` (midpoint in Pascal) |
| **Sound** | ROM BEEP routine at `$03B5`, frequency/duration calculated in Pascal with FP math |
| **Timing** | `Frames` reads 3-byte counter at `$5C78-$5C7A` (50 Hz). `Delay` counts frame interrupts |
| **Border** | `zx_border` via ROM routine |
| **Break check** | `__checkbreak`: reads Break+Space key combination |
| **Memory layout** | Program loads at `$8000`, heap between end-of-program and `$DFFF`, stack at `$DFFF` downward |

### No File I/O

The 48K target has no file system access. `files.pas` is not included.

---

## ZX Spectrum 128K (`zx128.pas` + `derby.pas` + `derby.asm`)

**Extends 48K** with bank switching for overlays.

### Composition

```
zx128.pas
  {$i system.pas}
  {$i zxrom.pas}
  {$i derby.pas}       → SelectBank + {$l derby.asm}
```

### Key Addition

```pascal
procedure SelectBank(Bank: Byte); register; external 'banksel_hl';
```

Pages 128K RAM banks (0-7) into the `$C000-$FFFF` slot. Used by the overlay system to swap overlay code in and out.

### No File I/O

Like the 48K target, the 128K target has no file system support.

---

## ZX Spectrum Next (`next.pas` + `esxdos.pas` + `tbblue.pas`)

**Full-featured target** — files via esxDOS, hardware registers via TBBlue, memory paging, overlays.

### Composition

```
next.pas
  {$i system.pas}
  {$i zxrom.pas}       → ZX screen/keyboard/sound/graphics
  {$i esxdos.pas}      → esxDOS file system + {$l esxdos.asm}
  {$i files.pas}       → Shared file abstraction
  {$i tbblue.pas}      → Next registers + {$l tbblue.asm}
```

### esxDOS File System

| Operation | esxDOS Call | Notes |
|-----------|------------|-------|
| Open | `$9a` (f_open) | Flags in BC: `$0300` (read), `$0f00` (create+write) |
| Close | `$9b` (f_close) | Handle in A |
| Read | `$9d` (f_read) | Handle in A, buffer in HL, count in BC |
| Write | `$9e` (f_write) | Handle in A, buffer in HL, count in BC |
| Seek | `$9f` (f_seek) | Handle in A, offset in BCDE |
| Delete | `$ad` (f_unlink) | Filename in HL |
| Rename | `$b0` (f_rename) | Old name in HL, new name in DE |
| File size | `$a1` (f_fstat) | 11-byte stat buffer, size at offset 7-8 |

### FileControlBlock (Next)

```pascal
FileControlBlock = record
  Handle: Byte;                   (* esxDOS file handle, 0=closed *)
  FileName: array[0..31] of Char; (* Null-terminated, 31 chars max *)
  RL: Integer;                    (* Current record pos (low 16 bits) *)
  RH: Byte;                       (* Current record pos (high 8 bits) *)
end;
```

### TBBlue / Next Registers

```pascal
function GetNextReg(Number: Integer): Integer;  (* Read Next register *)
procedure SetNextReg(Number, Value: Integer);    (* Write Next register *)
function GetCpuSpeed: Integer;                   (* Reg 7, bits 0-1 *)
procedure SetCpuSpeed(Value: Integer);           (* Reg 7, bits 0-1 *)
function GetMemPage(Slot: Byte): Byte;           (* 8K page mapping *)
procedure SetMemPage(Slot, Page: Byte);          (* 8K page mapping *)
```

---

## Agon Light / Console8 (`agon.pas` + `agon.asm` + `agonhead.asm`)

**Newest target** — MOS API for files, VDP for display, eZ80 CPU but running in Z80 compatibility mode.

### Composition

```
agon.pas
  {$l agonhead.asm}    → MOS header, startup, stack setup
  {$i system.pas}      → Layer 1 core + Layer 0
  {$l agon.asm}        → Agon-specific ASM
  ... VDP terminal, MOS API, command-line parsing ...
  {$i files.pas}       → Shared file abstraction
  ... graphics primitives ...
```

### Key Implementation Details

| Aspect | Implementation |
|--------|---------------|
| **Console output** | VDP character output via `__conout` |
| **Screen control** | VDP escape sequences for cursor, colour, clear |
| **Colour** | Agon colour constants (different mapping from ZX: Blue=4, Red=1, Green=2) |
| **File I/O** | MOS API: `$0A` (fopen), `$0B` (fclose), `$1A` (fread), `$1B` (fwrite), `$1C` (flseek) |
| **Key detection** | `__keypressed`, `__readkey` (MOS API) |
| **Command line** | `__getargc`, `__getargvchar` (MOS provides parsed argc/argv) |
| **Sound** | VDU sequence: `23, 0, $85, channel, 0, volume, freq_lo, freq_hi, dur_lo, dur_hi` |
| **Timing** | `__delay` (MOS-based), `Frames` reads 32-bit system timer (60 Hz) |
| **Graphics** | `al_plot`, `al_draw`, `al_circle`, `al_point` (VDP graphics commands) |
| **Overlays** | Single sparse overlay file loaded to physical `$50000`, copied to `$4E000-$4FFFF` |

### FileControlBlock (Agon)

```pascal
FileControlBlock = record
  Handle: Byte;                    (* MOS file handle, 0=closed *)
  FileName: array[0..63] of Char;  (* Null-terminated, 63 chars max *)
  RL: Integer;                     (* Current record position *)
  SL: Integer;                     (* File size in records *)
end;
```

Note the larger filename buffer (64 vs 32 on Next) and the inclusion of `SL` for file size (the Next computes size on demand via `f_fstat`).

### Differences from CP/M

Although the Agon shares some API patterns with CP/M (both have `Bdos`/`BdosHL` functions), the underlying implementation is completely different: CP/M uses BDOS trap calls (`call 5`), while Agon uses MOS API calls via a dedicated assembly wrapper.

---

## Cross-Platform Comparison

| Feature | CP/M | ZX48 | ZX128 | Next | Agon |
|---------|------|------|-------|------|------|
| Console I/O | BDOS | ROM | ROM | ROM | VDP |
| File I/O | BDOS FCB | -- | -- | esxDOS | MOS API |
| Screen control | VT52 | ROM | ROM | ROM | VDP |
| Colour | VT52 | Attr | Attr | Attr | VDP |
| Graphics | -- | ROM+ASM | ROM+ASM | ROM+ASM | VDP |
| Sound | -- | ROM | ROM | ROM | VDP |
| Overlays | -- | -- | Bank switch | Page map | Memory copy |
| Command line | $80 tail | -- | -- | -- | MOS argv |
| Keyboard | BDOS | Port | Port | Port | MOS |
| Timing | BDOS 141 | Frames | Frames | Frames | MOS timer |

## See Also

- [[pasta80/pasta80-overview]] — Project overview
- [[pasta80/pasta80-rtl-architecture]] — HAL pattern details
- [[pasta80/pasta80-lessons-for-rock]] — What Rock can learn from this
