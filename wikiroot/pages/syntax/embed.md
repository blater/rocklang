---
title: Embed Blocks
category: syntax
tags: [embed, inline-c, inline-asm, z80, interop]
sources: []
updated: 2026-04-11
status: current
---

# Embed Blocks

**Embed blocks** allow raw C or Z80 assembly to be inlined directly into a Rock source file. The lexer captures the block body verbatim. For `@embed c` the body is emitted unchanged into the C output; for `@embed asm` it is wrapped in a `#ifdef __SDCC` / `#asm` / `#endasm` guard so that host (gcc) builds skip it automatically.

## Syntax

```rock
@embed c
// Any valid C code here
int myHelper(int x) { return x * 2; }
@end c

@embed asm
; Any valid Z80 assembly here
ld a, 42
out (0xfe), a
@end asm
```

The language tag (`c` or `asm`) appears after both `@embed` and `@end` and must match.

## Inline C

Use `@embed c` to:
- Define C helper functions callable from Rock
- Use C types or macros not expressible in Rock
- Interface with external C libraries

```rock
@embed c
#include <stdio.h>

void c_hello(void) {
    printf("Hello from C!\n");
}
@end c

sub main(): void {
  c_hello();
}
```

## Inline Z80 Assembly

Use `@embed asm` for ZXN-specific low-level code. Assembly is passed through to the Z88DK compiler using its inline assembly syntax.

```rock
@embed asm
  ld hl, 0x4000   ; ZX Spectrum screen memory
  ld (hl), 0xff   ; Fill first byte
@end asm
```

**Note:** `@embed asm` targets ZXN. Host builds silently skip the block via the `#ifdef __SDCC` guard — see [Host Behaviour](#host-behaviour-for-embed-asm) below.

## Lexer Handling

The lexer recognises `@embed` and switches into a body-capture mode, scanning verbatim until it finds `@end <lang>`. The body (everything between) is stored in `token_t.embed_body`; the language tag in `token_t.embed_lang`. No parsing or validation of the body occurs.

## Generator Handling

Emit position is determined by scope depth at the point of the `@embed` block:

- **Top-level** (outside any `sub`): emitted in `generate_forward_defs`, before all function definitions.
- **Inline** (inside a `sub` or block): emitted in `generate_statement`, at the corresponding position in the function body.

The emitted form differs by language:

| Language | `is_function` | Emitted form |
|----------|---------------|--------------|
| `c` | 1 (top-level) | Body verbatim at file scope |
| `c` | 0 (inline) | Body wrapped in `{ ... }` |
| `asm` | either | `#ifdef __SDCC` / `#asm` / body / `#endasm` / `#endif` |

For `@embed asm`, the body is always wrapped unconditionally — the `#ifdef __SDCC` guard ensures gcc ignores the block entirely on host builds. The `#asm`/`#endasm` directives are processed by Z88DK's `zcc` frontend before SDCC ever sees the file.

## Host Behaviour for `@embed asm`

Host (gcc) builds do **not** reject `@embed asm`. The generated C contains:

```c
#ifdef __SDCC
#asm
  ld a, 42
  out (0xfe), a
#endasm
#endif
```

Because `__SDCC` is not defined when compiling with gcc, the entire block is preprocessed away. The surrounding function compiles and runs normally; the assembly instructions are never seen by the C compiler.

## Test Coverage

- `test/embed_test.rkr` — top-level `@embed c` defining a callable C function
- `test/embed_simple_test.rkr` — minimal top-level embed usage
- `test/embed_inline_test.rkr` — `@embed c` inside a function body modifying a local variable
- `test/embed_asm_test.rkr` — `@embed asm` inside a function; verifies compile-clean on host

See [[targets/zxn-z80]] for Z80 assembly context and [[lexer/lexer-overview]] for token handling detail.
