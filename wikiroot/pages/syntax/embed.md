---
title: Embed Blocks
category: syntax
tags: [embed, inline-c, inline-asm, z80, interop]
sources: []
updated: 2026-04-09
status: current
---

# Embed Blocks

**Embed blocks** allow raw C or Z80 assembly to be inlined directly into a Rock source file. The lexer captures the block body verbatim; the generator emits it unchanged into the C output.

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

**Note:** Inline assembly targets ZXN. Host builds reject it.

## Lexer Handling

The lexer recognises `@embed` and switches into a body-capture mode, scanning verbatim until it finds `@end <lang>`. The body (everything between) is stored in `token_t.embed_body`; the language tag in `token_t.embed_lang`. No parsing or validation of the body occurs.

## Generator Handling

The generator emits embed bodies directly to the output file at the position where the `@embed` block appears in the source. For top-level embed blocks, this is at the top of the C file (before function definitions). For inline embed blocks within a function body, the C code is emitted inline.

## Test Coverage

- `test/embed_test.rkr` — general embed block tests
- `test/embed_simple_test.rkr` — minimal embed usage
- `test/embed_inline_test.rkr` — embed block inside a function body

See [[targets/zxn-z80]] for Z80 assembly context and [[lexer/lexer-overview]] for token handling detail.
