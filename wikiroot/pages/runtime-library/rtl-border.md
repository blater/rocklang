---
title: "RTL Component: Border"
category: concepts
tags: [rtl, border, zxn, z88dk, builtins]
sources: [src/lib/border.h, src/lib/border.c, test/border_test.rkr]
updated: 2026-04-12
status: current
---

# RTL Component: Border

Second RTL component under the [[rtl-overview]] conventions. Sets the ZX Spectrum border colour by writing to port `$FE` via z88dk's `z80_outp`. The main purpose of this component in the RTL strategy is to **prove the C-only path** — no per-target `.asm` file, no exported data symbols, no SDCC underscore gymnastics.

## Rock-facing API

```rock
border(to_byte(2));              // red border
byte c := border_get();          // 2
```

| Name | Return | Args | Purpose |
|------|--------|------|---------|
| `border` | `void` | `byte colour` | Set border colour (low 3 bits). Writes port `$FE` on ZXN. |
| `border_get` | `byte` | none | Return the last colour passed to `border`. |

Colour constants in `src/lib/border.h` (`BORDER_BLACK`..`BORDER_WHITE` = 0..7) match the standard ZX ULA palette.

## Host vs ZXN

| Target | Behaviour |
|--------|-----------|
| Host (gcc) | `border()` masks to low 3 bits and stores in a static shadow. No hardware. |
| ZXN (SDCC) | `border()` masks, stores in the shadow, **and** calls `z80_outp(0xFE, colour)`. |

Both targets consult the shadow in `border_get()`. The shadow is authoritative on both platforms — the ZX ULA cannot read the border back from hardware, so keeping a software mirror is the normal pattern for any "write-only" hardware register.

## Implementation layout

| File | Purpose |
|------|---------|
| `src/lib/border.h` | Declarations + `BORDER_*` constants |
| `src/lib/border.c` | `#ifdef __SDCC` split: ZXN uses `<z80.h>` `z80_outp`; host is shadow-only |
| `test/border_test.rkr` | 3 assertions: set-red, set-green, high-bit masking |

**Notably absent: no `src/lib/zxn/border.asm`.** Components that only need port I/O or `<z80.h>` helpers don't need a separate assembly file — this is the first concrete proof that [[rtl-overview]] conventions degrade gracefully for simpler components.

## Compiler + build delta

Matches the recipe in [[rtl-overview]] exactly:

- `src/generator.c`: two `register_builtin` calls (`border`, `border_get`) and one `#include "border.h"` in `transpile()`.
- `rock`: one line appended to `RTL_C_SRCS`. `RTL_ZXN_ASM` is untouched.

Total compiler/build-script diff for the whole component: **~4 lines**.

## Verification

- `./run_tests.sh` — `border_test` contributes 3 assertions; total 224 host tests pass.
- `rock --target=zxn test/border_test.rkr` — produces `border_test.nex` (~33 KB) cleanly; no SDCC errors, only the pre-existing `Assert.rkr` "unused `this`" warnings.
- Manual hardware check (optional): on Next/CSpect, `border(BORDER_RED)` should flash the border red.

## Lesson for the convention set

This component validates a quiet but important corollary of convention #6 (SDCC underscore prefix): **if a component never exports asm symbols, the rule simply doesn't apply**. Writing `z80_outp` from C is indistinguishable from any other function call — no manual name mangling, no `PUBLIC` / `DEFC` bookkeeping. As a rule of thumb, prefer C + `<z80.h>` for simple port I/O; reserve `.asm` files for routines that are genuinely performance-sensitive (keyboard matrix scanning) or need access to registers C can't express.

## See Also

- [[rtl-overview]] — conventions and the SDCC underscore-prefix gotcha (not triggered here)
- [[rtl-keyboard]] — first component, shows the asm path
- [[targets/zxn/tools/z88dk-z80-library]] — `<z80.h>` reference including `z80_outp`
