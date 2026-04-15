---
title: "Rock RTL Overview and Conventions"
category: concepts
tags: [rtl, runtime, zxn, architecture, conventions]
sources: [src/lib/keyboard.h, src/lib/keyboard.c, src/lib/zxn/keyboard.asm, src/generator.c, rock]
updated: 2026-04-12
status: current
---

# Rock RTL Overview

Rock's runtime library (RTL) is a set of **per-component** C + optional Z80 assembly files under `src/lib/`, exposed to Rock programs as flat procedural **builtins**. The design is informed by the [[pasta80/pasta80-lessons-for-rock]] analysis and is being built up one component at a time, starting with a ZXN keyboard scanner as the pilot.

This page captures the conventions that every RTL component must follow. It is the rulebook — new components should read as minor variations of the keyboard component, not as fresh designs.

---

## Layering

```
┌─────────────────────────────────────────────────┐
│ Rock source (*.rkr)                             │
│   scan_keyboard();                              │
│   if key_pressed(KEY_5) != 0 then ...           │
├─────────────────────────────────────────────────┤
│ Generator (src/generator.c)                     │
│   register_builtin(..., "scan_keyboard", ...)   │
│   fprintf(f, "#include \"keyboard.h\"\n");     │
├─────────────────────────────────────────────────┤
│ Public header (src/lib/<name>.h)                │
│   Declarations + constant #defines              │
├─────────────────────────────────────────────────┤
│ C shim (src/lib/<name>.c)                       │
│   #ifdef __SDCC: delegate to asm                │
│   #else:         host stub / simulation         │
├─────────────────────────────────────────────────┤
│ Z80 asm (src/lib/zxn/<name>.asm)  — optional    │
│   SECTION code_user                             │
│   PUBLIC _foo / _FOO_BUFFER                     │
└─────────────────────────────────────────────────┘
```

## Conventions (locked in by the keyboard pilot)

1. **One component = one header + one C file + optional ASM.** Components are leaves; duplication among siblings is a smell, extract when it actually hurts. A component MAY consume another component's public header for its declared API. It MUST NOT reach into another's implementation file or duplicate state that belongs elsewhere.
2. **Host stub is mandatory.** Every ZXN component ships a host implementation (no-op, zero buffer, or simple simulation) so `./run_tests.sh` still passes on the host target.
3. **`#ifdef __SDCC` split lives in the `.c` file, never in the header.** Rock user code and the generator only ever see the declarations in the header.
4. **Flat procedural builtins only.** Expose functionality as top-level functions matching the `peek`/`poke` shape. No new keywords, no operator syntax, no namespaces per component.
5. **Constants as `#define` in the public header.** `KEY_5`, `KEY_COUNT`, etc. No enums (SDCC enum behaviour differs from gcc) and no Rock-side constants until proper `module`/constant syntax lands.
6. **SDCC underscore prefix is mandatory for any asm symbol referenced from C.** `extern void scan_keyboard(void)` binds to `_scan_keyboard` in the asm file, and an exported data buffer `FOO_BUFFER` must be declared `PUBLIC _FOO_BUFFER` in the asm. (See "Gotcha: SDCC symbol prefix" below.)
7. **`.asm` files use `SECTION code_user`** and keep their own `db`/`defb` data blocks alongside the code for now. bss placement can be revisited if a component grows large.
8. **Byte-oriented APIs.** Sizes, offsets, and counts in bytes. Avoid record/block abstractions until we meet a use case that needs them — the pasta80 `FileSeek` leak (see [[pasta80/pasta80-lessons-for-rock]] §8) is the cautionary tale.
9. **Return codes, not globals, for errors.** Not exercised by keyboard yet, but the rule applies from day one.
10. **Each component gets a test under `test/`** that exercises the host stub end-to-end. This is how regressions in the compiler/builtin/link path are caught before ZXN builds break silently.
11. **Lifecycle lives in [[rtl/rtl-host-caps]], not in components.** Any RTL component whose host implementation needs to open a terminal, probe `isatty`, call `tb_init`, install an `atexit` handler, or otherwise differ between ZXN (always-faithful) and host (probe-and-maybe-fall-back) MUST add a flag to `rock_host_caps` and read it on each call. Components are forbidden from calling lifecycle primitives directly; `rock_rtl_init()` (called once at startup by the generator-emitted `main()`) is the single place those run.

## Compiler and build plumbing

Adding a component `foo` requires **three small edits** and nothing else:

1. **`src/generator.c`** — in `new_generator()` (~line 341):
   ```c
   register_builtin(&res.table, "foo_do_something", "void");
   register_builtin(&res.table, "foo_query",        "byte");
   ```
   And in `transpile()` (~line 1842), append to the include block:
   ```c
   fprintf(f, "#include \"foo.h\"\n");
   ```
2. **`rock`** — add `foo.c` to `LIB_SRCS`; if there is assembly, add `foo.asm` to `ZXN_ASM`.
3. **`test/foo_test.rkr`** — host-stub test using `Assert.rkr`.

If a component needs a **host-only third-party dependency** (e.g. termbox2), add the single-TU implementation file under `src/lib/host/` and append its path to `RTL_HOST_SRCS` in `rock`. The gcc branch picks it up with any needed `-I` path; the SDCC branch ignores it. See [[rtl/rtl-print-at]] for the termbox2 example.

No parser changes. No lexer changes. No new AST nodes. The builtin dispatch path already supports zero-arg and byte-arg calls, so any function signature expressible with existing Rock types is free.

## Gotcha: SDCC symbol prefix

**Learned during the keyboard pilot (2026-04-12).** When SDCC emits C code it prefixes every extern identifier with a single underscore. A C declaration `extern unsigned char ZK_BUFFER[40];` resolves at link time as `_ZK_BUFFER`, so the `.asm` file must declare:

```
PUBLIC _ZK_BUFFER
DEFC _ZK_BUFFER = keybuffer
```

This is **not** optional — the first ZXN build of `keyboard_test.rkr` failed with `undefined symbol: _ZK_BUFFER` until both the `PUBLIC` and `DEFC` lines were prefixed. The same rule already applied to `_scan_keyboard` (the example asm file had it correct from the start), but data symbols are easy to overlook because C headers never show the mangling.

**Apply to:** every `PUBLIC` symbol in an RTL `.asm` file that is referenced from C via `extern`.

## Why flat builtins and not module syntax

A cleaner long-term design would expose each component as a Rock `module` with namespaced names (`keyboard.scan()`, `keyboard.pressed(KEY_5)`). Rock does not yet have module semantics wired through the type checker (see Session 28 notes in MEMORY.md), so the pilot takes the shortest path: flat builtins that bolt onto the existing `peek`/`poke` machinery with zero risk to the parser or name table. When module syntax lands, the RTL headers can be re-exposed as modules without changing either the C code or existing Rock programs (the flat names remain valid).

## Pilot component

- [[rtl/rtl-keyboard]] — ZX Spectrum 8×5 matrix scanner, first component, reference implementation for this page.
- [[rtl/rtl-host-caps]] — centralised host capability + lifecycle layer (rule 11).

## See Also

- [[pasta80/pasta80-lessons-for-rock]] — lessons this RTL strategy is built on (partially applied — see this page for the Rock-specific realisation)
- [[generator/generator-overview]] — where `register_builtin` and include emission live
- [[targets/zxn-z80]] — ZXN target toolchain notes
