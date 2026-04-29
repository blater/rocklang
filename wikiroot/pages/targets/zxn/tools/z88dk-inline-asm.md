---
title: Z88DK Inline Assembly and Calling Conventions
category: targets
tags: [zxn, z80, z88dk, sdcc, inline-asm, calling-convention, embed-asm]
sources: [z88dk_inlineAssembler.md, z88dk-calling-with-stack-params.md]
updated: 2026-04-11
status: current
---

# Z88DK Inline Assembly and Calling Conventions

This page covers how assembly is embedded inside C functions in Z88DK, and how parameters and return values flow across the C / assembly boundary. Both topics are directly relevant to Rock's `@embed asm` blocks. See [[syntax/embed]] for Rock syntax and [[targets/zxn-z80]] for the target overview.

## `#asm` / `#endasm` — Inline Assembly Blocks

Z88DK's `zcc` frontend pre-processes `#asm` / `#endasm` blocks before passing code to the backend compiler. This translation step means the same syntax works with both backend compilers:

```c
void flash_border(void) {
    #asm
        ld   a, 7
        out  (0xfe), a
    #endasm
}
```

Rock's generator wraps `@embed asm` bodies in exactly this form, guarded by `#ifdef __SDCC` so host (gcc) builds skip the block:

```c
void flash_border(void) {
#ifdef __SDCC
#asm
    ld   a, 7
    out  (0xfe), a
#endasm
#endif
}
```

### Inline Assembly in Macros

Using `#asm`/`#endasm` inside a C macro is unreliable. Use the `__asm__("")` pseudo-function instead:

```c
#define BORDER(n) __asm__("ld a," #n "\n out (0xfe),a")
```

### Z88DK Recommendation

Z88DK recommends **against** heavy inline assembly use because:
- The function prologue varies between backend compilers and calling conventions
- Inline blocks interrupt SDCC's optimiser
- SDCC may have allocated variables to registers at the point the `#asm` block runs

The preferred pattern for performance-critical assembly is to put the implementation in a separate `.asm` file, declare a C prototype, and link both. This also allows hand-crafted register-efficient entry points. See [[targets/zxn/tools/z80asm-reference]] for the z80asm assembler.

## Data Type Sizes

Z88DK (with SDCC backend, `-clib=sdcc_iy`) maps C types to Z80 widths:

| C Type | Rock Type | Size | Range |
|--------|-----------|------|-------|
| `char` / `unsigned char` | `byte` | 1 byte | 0–255 |
| `int` / `unsigned int` | `int` / `word` | 2 bytes | ±32767 / 0–65535 |
| `long` / `unsigned long` | `dword` | 4 bytes | ±2 147 483 647 |
| pointer | — | 2 bytes | 16-bit address space |

## Calling Convention (Stack-Based)

When a C function is compiled by Z88DK, parameters are pushed onto the Z80 stack before the call. At the point the function body begins executing, the stack layout is:

| SP offset | Content |
|-----------|---------|
| `SP` | Return address (2 bytes) |
| `SP+2` | Second parameter (2 bytes for `int`/pointer) |
| `SP+4` | First parameter (2 bytes for `int`/pointer) |

> **Note:** This layout applies to the sccz80 backend and to SDCC in stack-parameter mode. With `-clib=sdcc_iy` (what Rock uses), SDCC may pass the first few small arguments in registers rather than on the stack. For `@embed asm` blocks that run *inside* a function already compiled by SDCC, parameters have already been placed by the SDCC-generated prologue and you should not manipulate SP directly.

### Example: Inline Assembly as Full Function Body

This pattern (entire function body in `#asm`) is reliable for sccz80 or when SDCC is instructed to use stack-only parameters:

```c
void pokevalue(int pokeAddress, int pokeValue) {
    #asm
        pop  DE         ; save return address
        pop  BC         ; pokeValue (2nd param) → BC; use C
        pop  HL         ; pokeAddress (1st param) → HL
        ld   (HL), C    ; write byte at address
        push DE         ; restore return address
        ret
    #endasm
}
```

For `@embed asm` blocks that appear *mid-function* alongside Rock code, let SDCC manage the stack and registers — only issue instructions that do not alter SP or any register SDCC is tracking.

## Return Values

Z88DK returns values in fixed registers:

| Return type | Register(s) |
|-------------|-------------|
| `char` / `byte` | `L` (H = 0) |
| `int` / `word` / pointer | `HL` |
| `long` / `dword` | `DE:HL` (DE = high word) |

An assembly function that returns an `int` should leave the result in `HL` before `ret`.

## Safe Uses of `@embed asm` in Rock

Safe inline patterns that do not conflict with SDCC register tracking:

```rock
// Flash the border — LD A, n / OUT touches only A, no conflict
sub flash_border() {
  @embed asm
    ld  a, 7
    out (0xfe), a
  @end asm
}

// Delay loop — uses only B internally
sub short_delay() {
  @embed asm
    ld  b, 255
  .loop:
    djnz .loop
  @end asm
}
```

Avoid modifying `IY` inside `@embed asm` — Z88DK's SDCC backend uses `IY` as the frame pointer under `-clib=sdcc_iy`.

## See Also

- [[syntax/embed]] — `@embed asm` Rock syntax and generated output
- [[targets/zxn-z80]] — ZXN target overview
- [[targets/zxn/tools/z80asm-reference]] — z80asm assembler/linker reference
- [[targets/zxn/tools/z88dk-z80-library]] — z80.h C-callable library
