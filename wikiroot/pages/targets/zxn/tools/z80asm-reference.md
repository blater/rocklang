---
title: z80asm Assembler Reference
category: targets
tags: [zxn, z80, z80asm, z88dk, assembler, linker, directives, macros]
sources: [Tool---z80asm.md, Tool---z80asm---command-line.md, Tool---z80asm---input-format.md, Tool---z80asm---preprocessor.md, Tool---z80asm---expressions.md, Tool---z80asm---directives.md, Tool---z80asm---environment.md]
updated: 2026-04-11
status: current
---

# z80asm Assembler Reference

**z80asm** (`z88dk-z80asm`) is the relocatable macro assembler, linker, and librarian that forms the backend of the Z88DK C compilers. It assembles Z80-family and Intel 8080/8085 source files into relocatable object files, manages libraries, and links binaries. Rock programs compiled for the ZXN target go through z80asm internally — it is invoked by `zcc` after SDCC generates the intermediate assembly.

See [[targets/zxn-z80]] for the full ZXN compilation pipeline.

---

## Three Modes

```bash
# Assemble to .o object files
z88dk-z80asm [options] file.asm ...

# Assemble and link to binary
z88dk-z80asm -b [options] file.asm ...

# Build a library from object files
z88dk-z80asm -xmylib.lib [options] file.asm ...
```

Use a previously built library during linking with `-lmylib`.

---

## Key Command-Line Options

### Architecture

| Flag | Architecture |
|------|-------------|
| `-mz80` | Z80 (standard) |
| `-mz80n` | ZX Spectrum Next (Z80N — use for ZXN target) |
| `-mz80_strict` | Z80 without undocumented opcodes |

### Preprocessor

| Flag | Effect |
|------|--------|
| `-IPATH` | Add `PATH` to include search path |
| `-DSYMBOL[=VALUE]` | Define a symbol (default value = 1) |
| `-IXIY` | Swap all IX/IY references (useful when OS reserves one index register) |

### Output

| Flag | Effect |
|------|--------|
| `-b` | Assemble and link to binary (`.bin`) |
| `-oFILE` | Name the output file |
| `-ODIR` | Write output files in directory `DIR` |
| `-s` | Emit symbol table (`.sym`) |
| `-l` | Emit listing file (`.lis`) |
| `-m` | Emit address map (`.map`) |
| `-d` | Incremental assembly — skip if `.o` is newer than `.asm` |
| `-rADDR` | Relocate binary to address `ADDR` |

### Environment Variables

| Variable | Effect |
|----------|--------|
| `Z80ASM` | Space-separated default options applied to every invocation |
| `ZCCCFG` | Points to `zcc` configuration directory; z80asm searches the parent for libraries |

---

## Source File Format

### Comments

```asm
; Semicolon comment — preferred
// C++ style single-line comment
/* C style multi-line comment */
```

### Labels

```asm
.label      ; dot prefix — defines label at current address
label:      ; colon suffix — equivalent
```

### Local Labels

Local labels are scoped to the nearest preceding non-local label. Their full name is `parent@local`.

```asm
subroutine:
@loop:
    djnz @loop          ; jumps to subroutine@loop

sub2:
@loop:
    djnz @loop          ; jumps to sub2@loop

main:
    call subroutine@loop ; explicit full name
```

### Number Literals

| Format | Example |
|--------|---------|
| Decimal | `255`, `255d` |
| Hexadecimal | `$FF`, `0xFF`, `0FFh` |
| Binary | `%11`, `0b11`, `11b`, `@11` |
| Bitmap | `@"---##---"` (# = 1, - = 0) |
| Character | `'A'` |

Leading zeros are **not** octal (unlike C).

### Multiple Statements per Line

```asm
ld a, 1 \ ret    ; backslash separator
ld a, 1 : ret    ; colon separator (also accepted)
```

Line continuation:

```asm
ld a,\
1              ; → ld a, 1
```

### `ASMPC`

`ASMPC` expands to the address of the current instruction at link time.

---

## Expressions

Operator precedence (highest to lowest):

| Operators | Description |
|-----------|-------------|
| `+ - ! ~` `(...)` `[...]` | Unary, grouping |
| `**` | Power (right-associative) |
| `* / %` | Multiply / divide / remainder |
| `+ -` | Add / subtract |
| `<< >>` | Bit shift |
| `= == != <> < <= > >=` | Comparison |
| `&` | Bitwise AND |
| `\| ^` | Bitwise OR / XOR |
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `? :` | Ternary conditional |

---

## Preprocessor

### `#define` / `#undef`

C-style text macros with optional parameters:

```asm
#define BORDER(n)  ld a, n \ out (0xfe), a

BORDER(2)          ; expands to: ld a, 2 \ out (0xfe), a
```

Token pasting with `##`:

```asm
#define cat(a, b) a ## b
cat(reg, HL)       ; expands to: regHL
```

### `MACRO` / `ENDM`

Named macros with formal parameters:

```asm
MACRO push_all
    push bc
    push de
    push hl
ENDM

push_all           ; expands inline
```

Use `LOCAL` inside macros to create unique per-invocation labels:

```asm
MACRO wait_loop count
LOCAL done
    ld b, count
.loop: djnz .loop
ENDM
```

Use `EXITM` to exit a macro conditionally.

### `REPT` / `ENDR`

Repeat a block a fixed number of times:

```asm
REPT 4
    nop
ENDR
```

### `REPTC` / `REPTI`

Iterate over characters of a string (`REPTC`) or a list of values (`REPTI`):

```asm
REPTI reg, bc, de, hl
    push reg
ENDR
```

### `DEFL`

Mutable text substitution (unlike `#define`, can be redefined and self-referential):

```asm
DEFL counter = 0
DEFL counter = counter + 1   ; counter is now "0 + 1"
```

---

## Key Directives

### Data

| Directive | Aliases | Description |
|-----------|---------|-------------|
| `DEFB expr[,...]` | `DB`, `DEFM`, `DM`, `BYTE` | Byte(s) or string at current location |
| `DEFW expr[,...]` | `DW`, `WORD` | 16-bit word(s), little-endian |
| `DEFW_BE expr[,...]` | `DW_BE`, `DEFDB`, `DDB` | 16-bit word(s), big-endian |
| `DEFP expr[,...]` | `DP`, `PTR` | 24-bit word(s), little-endian |
| `DEFQ expr[,...]` | `DQ`, `DWORD` | 32-bit word(s), little-endian |
| `DEFS size[, fill]` | `DS` | Reserve `size` bytes, optionally filled |

String storage with fixed length:

```asm
DEFS 10, "hello"   ; "hello" + 5 zero bytes
```

### Symbol Definitions

```asm
DEFC name = expr          ; define a constant (may be address)
DEFINE sym[=val], ...     ; define as 1 (or value)
DEFGROUP { A, B=10, C }   ; C-enum style: A=0, B=10, C=11
DEFVARS $4000 {           ; C-struct style layout
    x DS.B 1              ; x = $4000
    y DS.W 1              ; y = $4001
}
```

### Layout

```asm
ORG $8000                 ; set origin of this section
SECTION code              ; start/switch section
ALIGN 256                 ; align section start to 256-byte boundary
PHASE $C000               ; assemble with addresses as if at $C000
DEPHASE                   ; end PHASE block
```

### Linking

```asm
PUBLIC myFunc             ; export symbol for use by other modules (alias: XDEF, XLIB)
EXTERN extFunc            ; import symbol from another module (alias: XREF, LIB)
INCLUDE "file.asm"        ; include another source file
GLOBAL sym                ; PUBLIC if defined locally, EXTERN otherwise
```

### Conditional Assembly

```asm
IF expr
    ; assembled if expr is true (non-zero)
ELIF expr2
    ; ...
ELSE
    ; ...
ENDIF

IFDEF SYMBOL ... ENDIF
IFNDEF SYMBOL ... ENDIF
```

Note: `#if`, `#ifdef`, `#ifndef` are also accepted (C preprocessor compatibility).

### Assertions

```asm
ASSERT expr, "message"    ; abort if expr is false at assembly time
```

### ZXN Copper Directives

These produce the encoded Copper instruction bytes directly:

| Directive | Encoding | Description |
|-----------|----------|-------------|
| `CU.WAIT ver, hor` | `0x8000 + (hor<<9) + ver` | Wait until raster position |
| `CU.MOVE reg, val` | `(reg<<8) + val` | Write `val` to Next register `reg` |
| `CU.STOP` | `0xFFFF` | Halt Copper execution |
| `CU.NOP` | `0x0000` | No-operation |

See [[targets/zxn/zxn-copper]] for the Copper co-processor.

### ZXN DMA Directives

`DMA.WR0` through `DMA.WR6` / `DMA.CMD` produce the encoded DMA write-register bytes. Parameters follow the zxnDMA specification (see [[targets/zxn/zxn-dma]]).

---

## File Types

| Extension | Purpose                                                 |
| --------- | ------------------------------------------------------- |
| `.asm`    | Assembly source file                                    |
| `.o`      | Relocatable object file                                 |
| `.bin`    | Linked binary output                                    |
| `.lis`    | Listing file (source + object bytes)                    |
| `.sym`    | Symbol table                                            |
| `.map`    | Address map with source locations                       |
| `.def`    | Global address definitions (includeable from other asm) |
| `.reloc`  | Relocation info for loaders                             |

---

## See Also

- [[targets/zxn-z80]] — ZXN target: Z88DK/SDCC pipeline
- [[targets/zxn/tools/z88dk-inline-asm]] — Inline assembly in C and calling conventions
- [[targets/zxn/tools/z88dk-z80-library]] — z80.h C-callable library
- [[targets/zxn/zxn-copper]] — Copper co-processor
- [[targets/zxn/zxn-dma]] — DMA engine
